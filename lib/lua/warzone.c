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

#include <SDL.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lib/framework/frame.h"

#include "warzone.h"

#include <physfs.h>

#include <SDL_thread.h>

const char *luaWZ_terminated = "__TERMINATED__";

typedef struct {
	PHYSFS_file *handle;
	char buffer[1024];
} physfsLuaChunkreaderState;

static const char *physfsLuaChunkreader(lua_State *L, void *data, size_t *size)
{
	physfsLuaChunkreaderState *state = (physfsLuaChunkreaderState *)data;

	if (PHYSFS_eof(state->handle))
	{
		return NULL;
	}
	*size = (size_t)PHYSFS_read(state->handle, state->buffer, 1, 1024);
	if (*size <= 0)
	{
		return NULL;
	}
	else
	{
		return state->buffer;
	}
}

int luaWZ_loadfile(lua_State *L, const char *filename)
{
	physfsLuaChunkreaderState state;
	int r;

	state.handle = PHYSFS_openRead(filename);
	if (state.handle == NULL)
	{
		lua_pushfstring(L, "\"%s\": %s", filename, PHYSFS_getLastError());
		return -1;
	}
	lua_pushfstring(L, "@%s", filename);
	r = lua_load(L, physfsLuaChunkreader, &state, lua_tostring(L, -1));
	lua_remove(L, -2);
	PHYSFS_close(state.handle);
	return r;
}

static int traceback(lua_State *L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  /* pass error message */
	lua_pushinteger(L, 2);  /* skip this function and traceback */
	lua_call(L, 2, 1);  /* call debug.traceback */
	return 1;
}

static int report (lua_State *L, int status)
{
	if (status && !lua_isnil(L, -1))
	{
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL)
		{
			msg = "(error object is not a string)";
		}
		if (strncmp(msg, luaWZ_terminated, sizeof(luaWZ_terminated)-1) == 0)
		{
			return 0;
		}
		debug( LOG_ERROR, "\n%s\n", msg);
		lua_pop(L, 1);
	}
	return status;
}

int luaWZ_pcall_backtrace(lua_State *L, int args, int ret)
{
	int status;
	int base = lua_gettop(L)-args;  /* function index */
	lua_pushcfunction(L, traceback);  /* push traceback function */
	lua_insert(L, base);  /* put it under chunk and args */

	status = report(L, lua_pcall(L, args, ret, base));

	lua_remove(L, base);  /* remove traceback function */
	/* force a complete garbage collection in case of errors */
	if (status != 0)
	{
		lua_gc(L, LUA_GCCOLLECT, 0);
	}
	return status;
}

static int luaWZ_pcall_backtrace_thread(void *param)
{
	lua_State *L = param;
	luaWZ_pcall_backtrace(L, 0, 0);

	return 0;
}

void luaWZ_pcall_backtrace_threaded(lua_State *L)
{
	SDL_CreateThread(luaWZ_pcall_backtrace_thread, L);
}

/*
 * Load and run a Lua chunk from a physfs file. Returns 1 on success.
 * Returns 0 and prints an error message to the pipmak terminal in case
 * of file loading, Lua compilation, or Lua runtime errors.
 */
int luaWZ_dofile_threaded(lua_State *L, const char *filename)
{
	if (luaWZ_loadfile(L, filename) != 0)
	{
		debug( LOG_ERROR, "loading Lua file: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return 0;
	}
	luaWZ_pcall_backtrace_threaded(L);
	return 1;
}

int luaWZ_dofile(lua_State *L, const char *filename)
{
	if (luaWZ_loadfile(L, filename) != 0)
	{
		debug( LOG_ERROR, "loading Lua file: %s ", lua_tostring(L, -1));
		lua_pop(L, 1);
		return 0;
	}
	luaWZ_pcall_backtrace(L, 0, 0);
	return 1;
}

bool luaL_checkboolean(lua_State* L, int param)
{
	luaL_checktype(L, param, LUA_TBOOLEAN);
	return (BOOL) lua_toboolean(L, param);
}

static int band(lua_State *L)
{
	int val1 = luaL_checkint(L,1);
	int val2 = luaL_checkint(L,2);
	lua_pushinteger(L, val1&val2);
	return 1;
}

static int dofile(lua_State *L)
{
	const char *filename = luaL_checkstring(L,1);
	if (!luaWZ_dofile(L, filename))
	{
		return luaL_error(L, "failed loading file");
	}
	return 0;
}

void luaWZ_openlibs(lua_State *L)
{
	lua_pushcfunction(L, band);
	lua_setglobal(L, "band");
	lua_pushcfunction(L, dofile);
	lua_setglobal(L, "dofile");
}

static void tag_error (lua_State *L, int narg, int tag)
{
	luaL_typerror(L, narg, lua_typename(L, tag));
}

int luaWZ_checkplayer (lua_State *L, int narg)
{
	lua_Integer d = lua_tointeger(L, narg);

	if (d == 0 && !lua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
	{
		tag_error(L, narg, LUA_TNUMBER);
	}
	if (d < 0 || d >= 8) // HACK: MAX_PLAYERS
	{
		return luaL_error(L, "player number out of range (%d)", d);
	}
	return d;
}

void luaWZ_setintfield (lua_State *L, const char *index, int value)
{
	lua_pushstring(L, index);
	lua_pushinteger(L, value);
	lua_settable(L, -3);
}

void luaWZ_setnumberfield (lua_State *L, const char *index, double value)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, value);
	lua_settable(L, -3);
}

void luaWZ_setpointerfield (lua_State *L, const char *index, void * ptr)
{
	lua_pushstring(L, index);
	lua_pushlightuserdata(L, ptr);
	lua_settable(L, -3);
}

void luaWZ_setstringfield (lua_State *L, const char *index, const char * string)
{
	lua_pushstring(L, index);
	lua_pushstring(L, string);
	lua_settable(L, -3);
}
