/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
/*
 * ScriptFuncs.c
 *
 * All the C functions callable from the script code
 *
 */
 
#include <time.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/stdio_ext.h"
#include "lib/widget/widget.h"

// Lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lib/lua/warzone.h"

#include "effects.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "lib/gamelib/gtime.h"
#include "lib/iniparser/iniparser.h"
#include "objects.h"
#include "hci.h"
#include "loadsave.h"
#include "message.h"
#include "challenge.h"
#include "intelmap.h"
#include "map.h"
#include "structure.h"
#include "display3d.h"
#include "research.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "power.h"
#include "console.h"
#include "scriptfuncs.h"
#include "geometry.h"
#include "visibility.h"
#include "gateway.h"
#include "drive.h"
#include "display.h"
#include "component.h"
#include "scriptextern.h"
#include "seqdisp.h"

#include "configuration.h"
#include "fpath.h"

#include "warzoneconfig.h"
#include "lighting.h"
#include "atmos.h"
#include "lib/sound/cdaudio.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multigifts.h"
#include "multilimit.h"
#include "advvis.h"
#include "mapgrid.h"
#include "lib/ivis_common/piestate.h"
#include "wrappers.h"
#include "order.h"
#include "orderdef.h"
#include "mission.h"
#include "loop.h"
#include "frontend.h"
#include "group.h"
#include "transporter.h"
#include "radar.h"
#include "levels.h"
#include "mission.h"
#include "projectile.h"
#include "cluster.h"
#include "multigifts.h"			//because of giftRadar()
#include "aiexperience.h"
#include "display3d.h"			//for showRangeAtPos()
#include "multimenu.h"
#include "lib/script/chat_processing.h"
#include "keymap.h"
#include "visibility.h"
#include "design.h"
#include "random.h"
#include "scriptobj.h"
#include "scriptvals.h"

static INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

// If this is defined then check max number of units not reached before adding more.
#define SCRIPT_CHECK_MAX_UNITS

static SDWORD	bitMask[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
static char		strParam1[MAXSTRLEN], strParam2[MAXSTRLEN];		//these should be used as string parameters for stackPopParams()

static bool	structHasModule(STRUCTURE *psStruct);

static DROID_TEMPLATE* scrCheckTemplateExists(SDWORD player, DROID_TEMPLATE *psTempl);

/// Hold the previously assigned player
static int nextPlayer = 0;
static Vector2i positions[MAX_PLAYERS];

void scriptSetStartPos(int position, int x, int y)
{
	positions[position].x = x;
	positions[position].y = y;
	debug(LOG_SCRIPT, "Setting start position %d to (%d, %d)", position, x, y);
}

BOOL scriptInit()
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		scriptSetStartPos(i, 0, 0);
	}
	nextPlayer = 0;
	return true;
}

static int scrScavengersActive(lua_State *L)
{
	lua_pushboolean(L, game.scavengers);
	return 1;
}

static int scrGetPlayer(lua_State *L)
{
	if (nextPlayer >= MAX_PLAYERS)
	{
		return luaL_error(L, "maximum number of players reached");
		// not reached
		return 0;
	}

	debug(LOG_SCRIPT, "Initialized player %d, starts at position (%d, %d), max %d players", nextPlayer, 
	      positions[nextPlayer].x, positions[nextPlayer].y, NetPlay.maxPlayers);
	lua_pushinteger(L, nextPlayer);
	nextPlayer++;
	return 1;
}

Vector2i getPlayerStartPosition(int player)
{
	return positions[player];
}

static int scrSafeDest(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);

	ASSERT_OR_RETURN(0, worldOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	lua_pushboolean(L, !(auxTile(map_coord(x), map_coord(y), player) & AUXBITS_DANGER));
	return 1;
}

static int scrThreatAt(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);

	ASSERT_OR_RETURN(0, worldOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	lua_pushboolean(L, auxTile(map_coord(x), map_coord(y), player) & AUXBITS_THREAT);
	return 1;
}

static int scrGetPlayerStartPosition(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int x = 0;
	int y = 0;

	if (player < NetPlay.maxPlayers)
	{
		x = positions[player].x;
		y = positions[player].y;
	}
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 2;
}

/******************************************************************************************/
/*				 Check for objects in areas											 */


// check for a base object being in range of a point
BOOL objectInRange(BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range)
{
	BASE_OBJECT		*psCurr;
	SDWORD			xdiff, ydiff, rangeSq;

	// See if there is a droid in range
	rangeSq = range * range;
	for(psCurr = psList; psCurr; psCurr = psCurr->psNext)
	{
		// skip partially build structures
		if ( (psCurr->type == OBJ_STRUCTURE) &&
			 (((STRUCTURE *)psCurr)->status != SS_BUILT) )
		{
			continue;
		}

		// skip flying vtols
		if ( (psCurr->type == OBJ_DROID) &&
			 isVtolDroid((DROID *)psCurr) &&
			 ((DROID *)psCurr)->sMove.Status != MOVEINACTIVE )
		{
			continue;
		}

		xdiff = (SDWORD)psCurr->pos.x - x;
		ydiff = (SDWORD)psCurr->pos.y - y;
		if (xdiff*xdiff + ydiff*ydiff < rangeSq)
		{
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------------------
// Check for any player object being within a certain range of a position
static int scrObjectInRange(lua_State *L)
{
	BOOL		found;
	
	int player = luaWZ_checkplayer(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);
	int range = luaL_checkint(L, 4);

	found = objectInRange((BASE_OBJECT *)apsDroidLists[player], x,y, range) ||
			objectInRange((BASE_OBJECT *)apsStructLists[player], x,y, range);

	lua_pushboolean(L, found);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Check for a droid being within a certain range of a position
static int scrDroidInRange(lua_State *L)
{
	BOOL		found;
	int player = luaWZ_checkplayer(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);
	int range = luaL_checkint(L, 4);

	found = objectInRange((BASE_OBJECT *)apsDroidLists[player], x,y, range);

	lua_pushboolean(L, found);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Check for a struct being within a certain range of a position
static int scrStructInRange(lua_State *L)
{
	BOOL		found;
	int player = luaWZ_checkplayer(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);
	int range = luaL_checkint(L, 4);

	found = objectInRange((BASE_OBJECT *)apsStructLists[player], x,y, range);

	lua_pushboolean(L, found);
	return 1;
}

// -----------------------------------------------------------------------------------------
static int scrPlayerPower(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);

	lua_pushinteger(L, getPower(player));
	return 1;
}


// -----------------------------------------------------------------------------------------
// check for a base object being in an area
static BOOL objectInArea(BASE_OBJECT *psList, SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	BASE_OBJECT		*psCurr;
	SDWORD			ox,oy;

	// See if there is a droid in Area
	for(psCurr = psList; psCurr; psCurr = psCurr->psNext)
	{
		// skip partially build structures
		if ( (psCurr->type == OBJ_STRUCTURE) &&
			 (((STRUCTURE *)psCurr)->status != SS_BUILT) )
		{
			continue;
		}

		ox = (SDWORD)psCurr->pos.x;
		oy = (SDWORD)psCurr->pos.y;
		if (ox >= x1 && ox <= x2 &&
			oy >= y1 && oy <= y2)
		{
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------------------
// Check for any player object being within a certain area
static int scrObjectInArea(lua_State *L)
{
	BOOL		found;
	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	found = objectInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2) ||
			objectInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	lua_pushboolean(L, found);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Check for a droid being within a certain area
static int scrDroidInArea(lua_State *L)
{
	BOOL		found;
	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	found = objectInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2);

	lua_pushboolean(L, found);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Check for a struct being within a certain Area of a position
static int scrStructInArea(lua_State *L)
{
	BOOL		found;
	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	found = objectInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	lua_pushboolean(L, found);
	return 1;
}


// -----------------------------------------------------------------------------------------
static int scrSeenStructInArea(lua_State *L)
{
	BOOL        found = false;
	STRUCTURE   *psCurr;
	SDWORD      ox,oy;
	
	int player = luaWZ_checkplayer(L, 1);
	int enemy  = luaL_checkint(L, 2);
	BOOL walls = luaL_checkboolean(L, 3);
	int x1 = luaL_checkint(L, 4);
	int y1 = luaL_checkint(L, 5);
	int x2 = luaL_checkint(L, 6);
	int y2 = luaL_checkint(L, 7);

	for(psCurr = apsStructLists[enemy]; psCurr; psCurr = psCurr->psNext)
	{
		// skip partially build structures
		if ( (psCurr->type == OBJ_STRUCTURE) && (((STRUCTURE *)psCurr)->status != SS_BUILT) )
		{
			continue;
		}

		// possible skip walls
		if(walls && (psCurr->pStructureType->type != REF_WALL  || psCurr->pStructureType->type !=REF_WALLCORNER))
		{
			continue;
		}

		ox = (SDWORD)psCurr->pos.x;
		oy = (SDWORD)psCurr->pos.y;
		if (ox >= x1 && ox <= x2 &&	oy >= y1 && oy <= y2)
		{
			// structure is in area.
			if(psCurr->visible[player])
			{
				found = true;
			}
		}
	}
	
	lua_pushboolean(L, found);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Check for a players structures but no walls being within a certain area
static int scrStructButNoWallsInArea(lua_State *L)
{
	SDWORD		ox,oy;
	STRUCTURE	*psStruct;
	SDWORD		found = false;

	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	for(psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((psStruct->pStructureType->type != REF_WALL) &&
			(psStruct->pStructureType->type != REF_WALLCORNER) &&
			(psStruct->status == SS_BUILT) )
		{
			ox = (SDWORD)psStruct->pos.x;
			oy = (SDWORD)psStruct->pos.y;
			if ((ox >= x1) && (ox <= x2) &&
				(oy >= y1) && (oy <= y2))
			{
				found = true;
				break;
			}
		}
	}

	lua_pushboolean(L, found);
	return 1;
}


// -----------------------------------------------------------------------------------------
// check for the number of base objects in an area
static SDWORD numObjectsInArea(BASE_OBJECT *psList, SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	BASE_OBJECT		*psCurr;
	SDWORD			ox,oy;
	SDWORD			count;

	// See if there is a droid in Area
	count = 0;
	for(psCurr = psList; psCurr; psCurr = psCurr->psNext)
	{
		// skip partially build structures
		if ( (psCurr->type == OBJ_STRUCTURE) &&
			 (((STRUCTURE *)psCurr)->status != SS_BUILT) )
		{
			continue;
		}

		ox = (SDWORD)psCurr->pos.x;
		oy = (SDWORD)psCurr->pos.y;
		if (ox >= x1 && ox <= x2 &&
			oy >= y1 && oy <= y2)
		{
			count += 1;
		}
	}

	return count;
}

// -----------------------------------------------------------------------------------------
// Count the number of player objects within a certain area
static int scrNumObjectsInArea(lua_State *L)
{
	SDWORD		count;

	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	count = numObjectsInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2) +
			numObjectsInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	lua_pushinteger(L, count);
	return 1;
}


// -----------------------------------------------------------------------------------------
// Count the number of player droids within a certain area
static int scrNumDroidsInArea(lua_State *L)
{
	SDWORD		count;

	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	count = numObjectsInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2);

	lua_pushinteger(L, count);
	return 1;
}


// -----------------------------------------------------------------------------------------
// Count the number of player structures within a certain area
static int scrNumStructsInArea(lua_State *L)
{
	SDWORD		count;

	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	count = numObjectsInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	lua_pushinteger(L, count);
	return 1;
}


// -----------------------------------------------------------------------------------------
// Count the number of player structures but not walls within a certain area
static int scrNumStructsButNotWallsInArea(lua_State *L)
{
	SDWORD		count, ox,oy;
	STRUCTURE	*psStruct;

	int player = luaWZ_checkplayer(L, 1);
	int x1 = luaL_checkint(L, 2);
	int y1 = luaL_checkint(L, 3);
	int x2 = luaL_checkint(L, 4);
	int y2 = luaL_checkint(L, 5);

	count = 0;
	for(psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((psStruct->pStructureType->type != REF_WALL) &&
			(psStruct->pStructureType->type != REF_WALLCORNER) &&
			(psStruct->status == SS_BUILT))
		{
			ox = (SDWORD)psStruct->pos.x;
			oy = (SDWORD)psStruct->pos.y;
			if ((ox >= x1) && (ox <= x2) &&
				(oy >= y1) && (oy <= y2))
			{
				count += 1;
			}
		}
	}

	lua_pushinteger(L, count);
	return 1;
}


// -----------------------------------------------------------------------------------------
// Count the number of structures in an area of a certain type
static int scrNumStructsByTypeInArea(lua_State *L)
{
	SDWORD		count, ox,oy;
	STRUCTURE	*psStruct;

	int player = luaWZ_checkplayer(L, 1);
	int type = luaL_checkint(L, 2);
	int x1 = luaL_checkint(L, 3);
	int y1 = luaL_checkint(L, 4);
	int x2 = luaL_checkint(L, 5);
	int y2 = luaL_checkint(L, 6);

	count = 0;
	for(psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((psStruct->pStructureType->type == (UDWORD)type) &&
			(psStruct->status == SS_BUILT))
		{
			ox = (SDWORD)psStruct->pos.x;
			oy = (SDWORD)psStruct->pos.y;
			if ((ox >= x1) && (ox <= x2) &&
				(oy >= y1) && (oy <= y2))
			{
				count += 1;
			}
		}
	}

	lua_pushinteger(L, count);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Check for a droid having seen a certain object
static int scrDroidHasSeen(lua_State *L)
{
	BASE_OBJECT	*psObj = luaWZObj_checkbaseobject(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	BOOL		seen;

	// See if any droid has seen this object
	seen = false;
	if (psObj->visible[player])
	{
		seen = true;
	}

	lua_pushboolean(L, seen);
	return 1;
}


// -----------------------------------------------------------------------------------------
// Enable a component to be researched
WZ_DECL_UNUSED static BOOL scrEnableComponent(void)
{
	SDWORD		player;
	INTERP_VAL	sVal;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}
	if (!stackPop(&sVal))
	{
		return false;
	}

	// enable the appropriate component
	switch (sVal.type)
	{
	case ST_BODY:
		apCompLists[player][COMP_BODY][sVal.v.ival] = FOUND;
		break;
	case ST_PROPULSION:
		apCompLists[player][COMP_PROPULSION][sVal.v.ival] = FOUND;
		break;
	case ST_ECM:
		apCompLists[player][COMP_ECM][sVal.v.ival] = FOUND;
		break;
	case ST_SENSOR:
		apCompLists[player][COMP_SENSOR][sVal.v.ival] = FOUND;
		break;
	case ST_CONSTRUCT:
		apCompLists[player][COMP_CONSTRUCT][sVal.v.ival] = FOUND;
		break;
	case ST_WEAPON:
		apCompLists[player][COMP_WEAPON][sVal.v.ival] = FOUND;
		break;
	case ST_REPAIR:
		apCompLists[player][COMP_REPAIRUNIT][sVal.v.ival] = FOUND;
		break;
	case ST_BRAIN:
		apCompLists[player][COMP_BRAIN][sVal.v.ival] = FOUND;
		break;
	default:
		ASSERT( false, "scrEnableComponent: unknown type" );
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
/// Make a component available
static int scrMakeComponentAvailable(lua_State *L)
{
	int index;

	const char *componentName = luaWZObj_checkname(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	index = getCompFromResName(COMP_BODY, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_BODY][index] = AVAILABLE;
		return 0;
	}
	index = getCompFromResName(COMP_PROPULSION, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_PROPULSION][index] = AVAILABLE;
		return 0;
	}
	index = getCompFromResName(COMP_ECM, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_ECM][index] = AVAILABLE;
		return 0;
	}
	index = getCompFromResName(COMP_SENSOR, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_SENSOR][index] = AVAILABLE;
		return 0;
	}
	index = getCompFromResName(COMP_CONSTRUCT, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_CONSTRUCT][index] = AVAILABLE;
		return 0;
	}
	index = getCompFromResName(COMP_WEAPON, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_WEAPON][index] = AVAILABLE;
		return 0;
	}
	index = getCompFromResName(COMP_REPAIRUNIT, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_REPAIRUNIT][index] = AVAILABLE;
		return 0;
	}
	index = getCompFromResName(COMP_BRAIN, componentName);
	if (index > 0)
	{
		apCompLists[player][COMP_BRAIN][index] = AVAILABLE;
		return 0;
	}

	return luaL_error(L, "component %s not found", componentName);
	// not reached
	return 0;
}

// -----------------------------------------------------------------------------------------
// Add a droid
static int scrAddDroidToMissionList(lua_State *L)
{
	DROID_TEMPLATE	*psTemplate = luaWZObj_checktemplate(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	DROID			*psDroid;

#ifdef SCRIPT_CHECK_MAX_UNITS
	// Don't build a new droid if player limit reached, unless it's a transporter.
	if( IsPlayerDroidLimitReached(player) && (psTemplate->droidType != DROID_TRANSPORTER) ) {
		debug( LOG_NEVER, "scrAddUnit : Max units reached ,player %d\n", player );
		psDroid = NULL;
	} else
#endif
	{
		psDroid = buildMissionDroid( psTemplate, 128, 128, player );
	}

	luaWZObj_pushdroid(L, psDroid);
	return 1;
}

// -----------------------------------------------------------------------------------------
/// Add a droid
static int scrAddDroid(lua_State *L)
{
	DROID			*psDroid;

	DROID_TEMPLATE	*psTemplate = luaWZObj_checktemplate(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);
	int player = luaL_checkint(L, 4);

#ifdef SCRIPT_CHECK_MAX_UNITS
	// Don't build a new droid if player limit reached, unless it's a transporter.
	if( IsPlayerDroidLimitReached(player) && (psTemplate->droidType != DROID_TRANSPORTER) ) {
		debug( LOG_NEVER, "scrAddUnit : Max units reached ,player %d\n", player );
		psDroid = NULL;
	} else
#endif
	{
		psDroid = buildDroid(psTemplate, x, y, player, false, NULL);
		if (psDroid)
		{
			addDroid(psDroid, apsDroidLists);
			debug( LOG_LIFE, "created droid for AI player %d %u", player, psDroid->id );
			if (isVtolDroid(psDroid))
			{
				// vtols start in the air
				moveMakeVtolHover(psDroid);
			}
		}
		else
		{
			debug(LOG_LIFE, "send droid create message to game queue for AI player %d", player );
		}
	}

	luaWZObj_pushdroid(L, psDroid);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Add droid to transporter
static int scrAddDroidToTransporter(lua_State *L)
{
	DROID *psTransporter = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 2, OBJ_DROID);

	if (!psTransporter || !psDroid)
		luaL_error(L, "Null unit passed");
	if (psTransporter->droidType == DROID_TRANSPORTER)
		luaL_error(L, "First unit is no transporter");

	/* check for space */
	if (checkTransporterSpace(psTransporter, psDroid))
	{
		if (droidRemove(psDroid, mission.apsDroidLists))
		{
			grpJoin(psTransporter->psGroup, psDroid);
		}
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
//check for a building to have been destroyed
WZ_DECL_UNUSED static BOOL scrBuildingDestroyed(void)
{
	SDWORD		player;
	UDWORD		structureID;
	BOOL		destroyed;
	STRUCTURE	*psCurr;

	if (!stackPopParams(2, ST_STRUCTUREID, &structureID, VAL_INT, &player))
	{
		return false;
	}
	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrBuildingDestroyed:player number is too high" );
		return false;
	}

	destroyed = true;
	//look thru the players list to see if the structure still exists
	for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->id == structureID)
		{
			destroyed = false;
		}
	}

	scrFunctionResult.v.bval = destroyed;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Enable a structure to be built
static int scrEnableStructure(lua_State *L)
{
	int index = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	if (index < (SDWORD)0 || index > (SDWORD)numStructureStats)
	{
		return luaL_error(L, "invalid structure stat" );
	}
	
	// enable the appropriate structure
	apStructTypeLists[player][index] = AVAILABLE;
	return 0;
}



// -----------------------------------------------------------------------------------------
// Check if a structure can be built.
// currently PC skirmish only.
static int scrIsStructureAvailable(lua_State *L)
{
	int index = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	if (index < 0)
	{
		return luaL_error(L, "unknown structure type");
	}
	lua_pushboolean(L, apStructTypeLists[player][index] == AVAILABLE
	                   && asStructLimits[player][index].currentQuantity < asStructLimits[player][index].limit);
	return 1;
}


// -----------------------------------------------------------------------------------------
//make the droid with the matching id the currently selected droid
WZ_DECL_UNUSED static BOOL scrSelectDroidByID(void)
{
	SDWORD			player, droidID;
	BOOL			selected;

	if (!stackPopParams(2, ST_DROIDID, &droidID, VAL_INT, &player))
	{
		return false;
	}

	selected = false;
	if (selectDroidByID(droidID, player))
	{
		selected = true;
	}

	//store the result cos might need to check the droid exists before doing anything else
	scrFunctionResult.v.bval = selected;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}


// -----------------------------------------------------------------------------------------
// Pop up a message box with a number value in it
WZ_DECL_UNUSED static BOOL scrNumMB(void)
{
	SDWORD	val;

	if (!stackPopParams(1, VAL_INT, &val))
	{
		return false;
	}

	debug(LOG_NEVER, "called by script with value: %d", val);

	return true;
}


// -----------------------------------------------------------------------------------------
// Do an approximation to a square root
WZ_DECL_UNUSED static BOOL scrApproxRoot(void)
{
	SDWORD	val1, val2;

	if (!stackPopParams(2, VAL_INT, &val1, VAL_INT, &val2))
	{
		return false;
	}

	if (val1 < val2)
	{
		scrFunctionResult.v.ival = val2 + (val1 >> 1);
	}
	else
	{
		scrFunctionResult.v.ival = val1 + (val2 >> 1);
	}

	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// Add a reticule button to the interface
static int scrAddReticuleButton(lua_State *L)
{
	int val = luaL_checkint(L, 1);

	if (selfTest)
	{
		return 0;	// hack to prevent crashing in self-test
	}

	//set the appropriate flag to 'draw' the button
	switch (val)
	{
	case IDRET_OPTIONS:
		// bit of a hack here to keep compatibility with old scripts
		widgReveal(psWScreen, IDRET_COMMAND);
		break;
	case IDRET_COMMAND:
		widgReveal(psWScreen, IDRET_COMMAND);
		break;
	case IDRET_BUILD:
		widgReveal(psWScreen, IDRET_BUILD);
		break;
	case IDRET_MANUFACTURE:
		widgReveal(psWScreen, IDRET_MANUFACTURE);
		break;
	case IDRET_RESEARCH:
		widgReveal(psWScreen, IDRET_RESEARCH);
		break;
	case IDRET_INTEL_MAP:
		widgReveal(psWScreen, IDRET_INTEL_MAP);
		break;
	case IDRET_DESIGN:
		widgReveal(psWScreen, IDRET_DESIGN);
		break;
	case IDRET_CANCEL:
		widgReveal(psWScreen, IDRET_CANCEL);
		break;
	default:
		return luaL_error(L, "Invalid reticule Button ID" );
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
//Remove a reticule button from the interface
static int scrRemoveReticuleButton(lua_State *L)
{
	int val = luaL_checkint(L, 1);
	BOOL bReset = luaL_checkboolean(L,2);

	if (selfTest)
	{
		return 0;
	}

	if(bInTutorial)
	{
		if(bReset)	// not always desirable
		{
			intResetScreen(true);
		}
	}
	switch (val)
	{
	case IDRET_OPTIONS:
		// bit of a hack here to keep compatibility with old scripts
		widgHide(psWScreen, IDRET_COMMAND);
		break;
	case IDRET_COMMAND:
		widgHide(psWScreen, IDRET_COMMAND);
		break;
	case IDRET_BUILD:
		widgHide(psWScreen, IDRET_BUILD);
		break;
	case IDRET_MANUFACTURE:
		widgHide(psWScreen, IDRET_MANUFACTURE);
		break;
	case IDRET_RESEARCH:
		widgHide(psWScreen, IDRET_RESEARCH);
		break;
	case IDRET_INTEL_MAP:
		widgHide(psWScreen, IDRET_INTEL_MAP);
		break;
	case IDRET_DESIGN:
		widgHide(psWScreen, IDRET_DESIGN);
		break;
	case IDRET_CANCEL:
		widgHide(psWScreen, IDRET_CANCEL);
		break;
	default:
		return luaL_error(L, "Invalid reticule Button ID" );
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
// add a message to the Intelligence Display
static int scrAddMessage(lua_State *L)
{
	MESSAGE			*psMessage;
	VIEWDATA	*psViewData;
	UDWORD			height;

	const char *message = luaL_checkstring(L, 1);
	int msgType = luaL_checkint(L, 2);
	int player = luaWZ_checkplayer(L, 3);
	BOOL playImmediate = luaL_checkboolean(L, 4);

	psViewData = getViewData(message);

	//create the message
	psMessage = addMessage(msgType, false, player);
	if (psMessage)
	{
		//set the data
		psMessage->pViewData = (MSG_VIEWDATA*)psViewData;
		debug(LOG_MSG, "Adding %s pViewData=%p", psViewData->pName, psMessage->pViewData);
		if (msgType == MSG_PROXIMITY)
		{
			//check the z value is at least the height of the terrain
			height = map_Height(((VIEW_PROXIMITY *)psViewData->pData)->x,
				((VIEW_PROXIMITY *)psViewData->pData)->y);
			if (((VIEW_PROXIMITY *)psViewData->pData)->z < height)
			{
				((VIEW_PROXIMITY *)psViewData->pData)->z = height;
			}
		}

		if (playImmediate && !selfTest)
		{
			displayImmediateMessage(psMessage);
			stopReticuleButtonFlash(IDRET_INTEL_MAP);
		}
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
// remove a message from the Intelligence Display
static int scrRemoveMessage(lua_State *L)
{
	MESSAGE			*psMessage;
	VIEWDATA		*psViewData;

	const char *message = luaL_checkstring(L, 1);
	int msgType = luaL_checkint(L, 2);
	int player = luaWZ_checkplayer(L, 3);
	psViewData = getViewData(message);

	//find the message
	psMessage = findMessage((MSG_VIEWDATA*)psViewData, msgType, player);
	if (psMessage)
	{
		//delete it
		debug(LOG_MSG, "Removing %s", psViewData->pName);
		removeMessage(psMessage, player);
	}
	else
	{
		return luaL_error(L, "cannot find message - %s", psViewData->pName );
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
/**builds a droid in the specified factory*/
static int scrBuildDroid(lua_State *L)
{
	DROID_TEMPLATE	*psTemplate = luaWZObj_checktemplate(L, 1);
	STRUCTURE *psFactory = (STRUCTURE*)luaWZObj_checkobject(L, 2, OBJ_STRUCTURE);
	// note, argument 3 (player) is unused
	int productionRun = luaL_checkint(L, 4);
	
	if (productionRun > UBYTE_MAX)
	{
		return luaL_error(L, "scrBuildUnit: production run too high" );
	}

	if (!(psFactory->pStructureType->type == REF_FACTORY ||
		psFactory->pStructureType->type == REF_CYBORG_FACTORY ||
		psFactory->pStructureType->type == REF_VTOL_FACTORY))
	{
		return luaL_error(L, "scrBuildUnit: structure is not a factory" );
	}
	if (psTemplate == NULL)
	{
		return luaL_error(L, "scrBuildUnit: Invalid template pointer" );
	}

	//check building the right sort of droid for the factory
	if (!validTemplateForFactory(psTemplate, psFactory))
	{

		return luaL_error(L, "scrBuildUnit: invalid template - %s for factory - %s",
	                 psTemplate->aName, psFactory->pStructureType->pName);
	}

	if (productionRun != 1)
	{
		debug(LOG_ERROR, "A script is trying to build a different number (%d) than 1 droid.", productionRun);
	}
	structSetManufacture(psFactory, psTemplate, ModeQueue);

	return 0;
}

// -----------------------------------------------------------------------------------------
/// for a specified structure, set the assembly point droids go to when built
static int scrSetAssemblyPoint(lua_State *L)
{
	STRUCTURE *psBuilding = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);

	if (psBuilding->pStructureType->type != REF_FACTORY &&
		psBuilding->pStructureType->type != REF_CYBORG_FACTORY &&
		psBuilding->pStructureType->type != REF_VTOL_FACTORY)
	{
		return luaL_error(L, "structure is not a factory" );
	}

	setAssemblyPoint(((FACTORY *)psBuilding->pFunctionality)->psAssemblyPoint,x,y,
		psBuilding->player, true);

	return 0;
}

// -----------------------------------------------------------------------------------------
/// test for structure is idle or not
static int scrStructureIdle(lua_State *L)
{
	STRUCTURE *psBuilding = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);
	BOOL		idle;

	idle = structureIdle(psBuilding);
	lua_pushboolean(L, idle);
	return 1;
}

// -----------------------------------------------------------------------------------------
// sends a players droids to a location to attack
WZ_DECL_UNUSED static BOOL	scrAttackLocation(void)
{
	SDWORD		player, x, y;

	if (!stackPopParams(3, VAL_INT, &x, VAL_INT, &y, VAL_INT, &player))
	{
		return false;
	}

	debug(LOG_ERROR, "UNUSED FUNCTION attackLocation called - do not use!");

	return true;
}

// -----------------------------------------------------------------------------------------
/// Destroy a feature
static int scrDestroyFeature(lua_State *L)
{
	FEATURE *psFeature = (FEATURE*)luaWZObj_checkobject(L, 1, OBJ_FEATURE);

	removeFeature(psFeature);
	return 0;
}

// -----------------------------------------------------------------------------------------
//Add a feature
static int scrAddFeature(lua_State *L)
{
	FEATURE_STATS	*psStat;
	FEATURE			*psFeat = NULL;
	SDWORD			iMapX, iMapY, iTestX, iTestY, iFeat;
	
	const char *featureName = luaL_checkstring(L, 1);
	int iX = luaL_checkint(L, 2);
	int iY = luaL_checkint(L, 3);

	iFeat = getFeatureStatFromName(featureName);
	psStat = (FEATURE_STATS *)(asFeatureStats + iFeat);

	ASSERT( psStat != NULL,
			"scrAddFeature: Invalid feature pointer" );

	if ( psStat != NULL )
	{
		iMapX = map_coord(iX);
		iMapY = map_coord(iY);

		/* check for wrecked feature already on-tile and remove */
		for(psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
		{
			iTestX = map_coord(psFeat->pos.x);
			iTestY = map_coord(psFeat->pos.y);

			if ( (iTestX == iMapX) && (iTestY == iMapY) )
			{
				if ( psFeat->psStats->subType == FEAT_BUILD_WRECK )
				{
					/* remove feature */
					removeFeature( psFeat );
					break;
				}
				else
				{
					ASSERT( false,
					"scrAddFeature: building feature on tile already occupied\n" );
				}
			}
		}

		psFeat = buildFeature( psStat, iX, iY, false );
	}

	luaWZObj_pushfeature(L, psFeat);
	return 1;
}

// -----------------------------------------------------------------------------------------
//Add a structure
WZ_DECL_UNUSED static BOOL scrAddStructure(void)
{
	STRUCTURE_STATS		*psStat;
	STRUCTURE			*psStruct = NULL;
	SDWORD				iX, iY, iMapX, iMapY;//, iWidth, iBreadth;
	SDWORD				iStruct, iPlayer;//, iW, iB;

	if ( !stackPopParams( 4, ST_STRUCTURESTAT, &iStruct, VAL_INT, &iPlayer,
							 VAL_INT, &iX, VAL_INT, &iY ) )
	{
		return false;
	}

	psStat = (STRUCTURE_STATS *)(asStructureStats + iStruct);

	ASSERT( psStat != NULL,
			"scrAddStructure: Invalid feature pointer" );

	if ( psStat != NULL )
	{
		/* offset coords so building centre at (iX, iY) */
/*		no longer necessary - buildStruct no longer uses top left
		iX -= psStat->baseWidth*TILE_UNITS/2;
		iY -= psStat->baseBreadth*TILE_UNITS/2;*/

		iMapX = map_coord(iX);
		iMapY = map_coord(iY);

		/* check for structure already on-tile */
		if(TileHasStructure(mapTile(iMapX,iMapY)))
		{
			ASSERT( false,
			"scrAddStructure: tile already occupied by structure\n" );
		}

		psStruct = buildStructure( psStat, iX, iY, iPlayer, false );
		if ( psStruct != NULL )
		{
			psStruct->status = SS_BUILT;
			buildingComplete(psStruct);

			/*
			Apart from this being wrong (iWidth = 0 when psStat->baseWidth = 1
			and you end up in an infinite loop) we don't need to do this here
			since the map is flattened as part of buildStructure

			iWidth   = psStat->baseWidth/2;
			iBreadth = psStat->baseBreadth/2;

			// flatten tiles across building base
			for ( iW=iMapX; iW<=iMapX+(SDWORD)psStat->baseWidth; iW+=iWidth )
			{
				for ( iB=iMapY; iB<=iMapY+(SDWORD)psStat->baseBreadth; iB+=iBreadth )
				{
					setTileHeight(iW, iB, psStruct->pos.z);
				}
			}*/
		}
	}

	scrFunctionResult.v.oval = psStruct;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//Destroy a structure
static int scrDestroyStructure(lua_State *L)
{
	STRUCTURE *psStruct = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);
	destroyStruct(psStruct);
	return 0;
}

// -----------------------------------------------------------------------------------------
//NEXT 2 FUNCS ONLY USED IN MULTIPLAYER AS FAR AS I KNOW (25 AUG98) alexl.
//for the bucket version
static	STRUCTURE_STATS	*psStructStatToFindB[MAX_PLAYERS];
static	UDWORD			playerToEnumStructB[MAX_PLAYERS];
static	UDWORD			enumStructCountB[MAX_PLAYERS];
static	BOOL			structfindanyB[MAX_PLAYERS];
static	SDWORD			playerVisibleStructB[MAX_PLAYERS];		//player whose structures must be visible

// init enum visible structures - takes bucket as additional parameter
WZ_DECL_UNUSED static BOOL scrInitEnumStructB(void)
{
	SDWORD		lookingPlayer,iStat,targetPlayer,bucket;
	BOOL		any;

	if ( !stackPopParams(5,VAL_BOOL,&any, ST_STRUCTURESTAT, &iStat,
		VAL_INT, &targetPlayer, VAL_INT, &lookingPlayer, VAL_INT, &bucket) )
	{
		return false;
	}

	ASSERT(targetPlayer >= 0 && targetPlayer < MAX_PLAYERS,
		"scrInitEnumStructB: targetPlayer out of bounds: %d", targetPlayer);

	ASSERT(lookingPlayer >= 0 && lookingPlayer < MAX_PLAYERS,
		"scrInitEnumStructB: lookingPlayer out of bounds: %d", lookingPlayer);

	ASSERT(bucket >= 0 && bucket < MAX_PLAYERS,
		"scrInitEnumStructB: bucket out of bounds: %d", bucket);

	/* Any structure type regardless of the passed type? */
	structfindanyB[bucket] = any;

	psStructStatToFindB[bucket]	= (STRUCTURE_STATS *)(asStructureStats + iStat);
	playerToEnumStructB[bucket]	= (UDWORD)targetPlayer;
	playerVisibleStructB[bucket] = lookingPlayer;		//remember who must be able to see the structure
	enumStructCountB[bucket] = 0;

	return true;
}

// Similar to scrEnumStruct, but uses bucket
WZ_DECL_UNUSED static BOOL scrEnumStructB(void)
{
	SDWORD		bucket;
	UDWORD		count;
	STRUCTURE	*psStruct;

	if ( !stackPopParams(1, VAL_INT, &bucket) )
	{
		return false;
	}

	ASSERT(bucket >= 0 && bucket < MAX_PLAYERS,
		"scrEnumStructB: bucket out of bounds: %d", bucket);

	// go to the correct start point in the structure list.
	count = 0;
	for(psStruct=apsStructLists[playerToEnumStructB[bucket]];psStruct && count<enumStructCountB[bucket];count++)
	{
		psStruct = psStruct->psNext;
	}

	if(psStruct == NULL)		// no more to find.
	{
		scrFunctionResult.v.oval = NULL;
		if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
		{
			return false;
		}
		return true;
	}

	while(psStruct)	// find a visible structure of required type.
	{
		if(	(structfindanyB[bucket] || (psStruct->pStructureType->ref == psStructStatToFindB[bucket]->ref))
			&&
			((playerVisibleStructB[bucket] < 0) || (psStruct->visible[playerVisibleStructB[bucket]]))	//perform visibility test
			)
		{
			scrFunctionResult.v.oval = psStruct;
			if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))			//	push scrFunctionResult
			{
				return false;
			}
			enumStructCountB[bucket]++;

			return true;
		}
		enumStructCountB[bucket]++;
		psStruct = psStruct->psNext;
	}
	// push NULL, none found;
	scrFunctionResult.v.oval = NULL;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/**looks to see if a structure (specified by type) exists and is being built*/
static int scrStructureBeingBuilt(lua_State *L)
{
	STRUCTURE_STATS		*psStats;
	BOOL				beingBuilt;
	
	int index = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	psStats = (STRUCTURE_STATS *)(asStructureStats + index);
	beingBuilt = false;
	if (checkStructureStatus(psStats, player, SS_BEING_BUILT))
	{
		beingBuilt = true;
	}

	lua_pushboolean(L, beingBuilt);
	return 1;
}



// -----------------------------------------------------------------------------------------
// multiplayer skirmish only for now.
/// returns true if a specific struct is complete. I know it's like the previous func,
static int scrStructureComplete(lua_State *L)
{
	STRUCTURE *psStruct = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);

	lua_pushboolean(L, psStruct->status == SS_BUILT);
	return 1;
}

// -----------------------------------------------------------------------------------------
/// looks to see if a structure (specified by type) exists and built
static int scrStructureBuilt(lua_State *L)
{
	STRUCTURE_STATS *psStats;

	int structInc = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	ASSERT_OR_RETURN( false, structInc < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", structInc, numStructureStats);
	psStats = (STRUCTURE_STATS *)(asStructureStats + structInc);

	lua_pushboolean(L, checkStructureStatus(psStats, player, SS_BUILT));
	return 1;
}

// -----------------------------------------------------------------------------------------
/*centre the view on an object - can be droid/structure or feature */
WZ_DECL_UNUSED static BOOL scrCentreView(void)
{
	BASE_OBJECT	*psObj;
//	INTERP_VAL	sVal;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		return false;
	}

	if (psObj == NULL)
	{
		ASSERT( false, "scrCentreView: NULL object" );
		return false;
	}

	//centre the view on the objects x/y
	setViewPos(map_coord(psObj->pos.x), map_coord(psObj->pos.y), false);

	return true;
}

// -----------------------------------------------------------------------------------------
/*centre the view on a position */
static int scrCentreViewPos(lua_State *L)
{
	int x,y;
	
	x = luaL_checkint(L,1);
	y = luaL_checkint(L,2);

	if ( (x < 0) || (x >= (SDWORD)mapWidth*TILE_UNITS) ||
		 (y < 0) || (y >= (SDWORD)mapHeight*TILE_UNITS))
	{
		return luaL_error(L, "coords off map" );
	}

	//centre the view on the objects x/y
	setViewPos(map_coord(x), map_coord(y), false);

	return 0;
}

static STRUCTURE *unbuiltIter = NULL;
static int unbuiltPlayer = -1;

WZ_DECL_UNUSED static BOOL scrEnumUnbuilt(void)
{
	if (!stackPopParams(1, VAL_INT, &unbuiltPlayer))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, unbuiltPlayer < MAX_PLAYERS && unbuiltPlayer >= 0, "Player number %d out of bounds", unbuiltPlayer);
	unbuiltIter = apsStructLists[unbuiltPlayer];
	return true;
}

WZ_DECL_UNUSED static BOOL scrIterateUnbuilt(void)
{
	for (; unbuiltIter && unbuiltIter->status != SS_BEING_BUILT; unbuiltIter = unbuiltIter->psNext) ;
	scrFunctionResult.v.oval = unbuiltIter;
	if (unbuiltIter)
	{
		unbuiltIter = unbuiltIter->psNext;	// proceed to next, so we do not report same twice (or infinitely loop)
	}
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/// Get a pointer to a structure based on a stat - returns nil if cannot find one
static int scrGetStructure(lua_State *L)
{
	STRUCTURE			*psStruct;
	UDWORD				structType;
	
	int index = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	structType = asStructureStats[index].ref;

	//search the players' list of built structures to see if one exists
	for (psStruct = apsStructLists[player]; psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->pStructureType->ref == structType)
		{
			// found it!
			luaWZObj_pushstructure(L, psStruct);
			return 1;
		}
	}

	// no structure of this type found
	lua_pushnil(L);
	return 1;
}

// -----------------------------------------------------------------------------------------
// Get a pointer to a template based on a component stat - returns NULL if cannot find one
WZ_DECL_UNUSED static BOOL scrGetTemplate(void)
{
	SDWORD				player;
	DROID_TEMPLATE		*psTemplate;
	BOOL				found;
	INTERP_VAL			sVal;
	UDWORD				i;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
//		return luaL_error(L, "invalid player number" );
	}

	if (!stackPop(&sVal))
	{
		return false;
	}

	//search the players' list of templates to see if one exists
	found = false;
	for (psTemplate = apsDroidTemplates[player]; psTemplate != NULL; psTemplate =
		psTemplate->psNext)
	{
		switch( sVal.type)
		{
		case ST_BODY:
			if (psTemplate->asParts[COMP_BODY] == sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_PROPULSION:
			if (psTemplate->asParts[COMP_PROPULSION] == sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_ECM:
			if (psTemplate->asParts[COMP_ECM] == sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_SENSOR:
			if (psTemplate->asParts[COMP_SENSOR] == sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_CONSTRUCT:
			if (psTemplate->asParts[COMP_CONSTRUCT] == sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_REPAIR:
			if (psTemplate->asParts[COMP_REPAIRUNIT] == sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_WEAPON:
			for (i=0; i < DROID_MAXWEAPS; i++)
			{
				if (psTemplate->asWeaps[i] == (UDWORD)sVal.v.ival)
				{
					found = true;
					break;
				}
			}
			break;
		default:
			ASSERT( false, "scrGetTemplate: unknown type" );
			return false;
		}

		if (found)
		{
			break;
		}
	}

	//make sure pass NULL back if not got one
	if (!found)
	{
		psTemplate = NULL;
	}

	scrFunctionResult.v.oval = psTemplate;
	if (!stackPushResult((INTERP_TYPE)ST_TEMPLATE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Get a pointer to a droid based on a component stat - returns NULL if cannot find one
WZ_DECL_UNUSED static BOOL scrGetDroid(void)
{
	SDWORD				player;
	DROID				*psDroid;
	BOOL				found;
	INTERP_VAL			sVal;
	UDWORD				i;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
//		return luaL_error(L, "invalid player number" );
	}

	if (!stackPop(&sVal))
	{
		return false;
	}

	//search the players' list of droid to see if one exists
	found = false;
	for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid =
		psDroid->psNext)
	{
		switch( sVal.type)
		{
		case ST_BODY:
			if (psDroid->asBits[COMP_BODY].nStat == (UDWORD)sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_PROPULSION:
			if (psDroid->asBits[COMP_PROPULSION].nStat == (UDWORD)sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_ECM:
			if (psDroid->asBits[COMP_ECM].nStat == (UDWORD)sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_SENSOR:
			if (psDroid->asBits[COMP_SENSOR].nStat == (UDWORD)sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_CONSTRUCT:
			if (psDroid->asBits[COMP_CONSTRUCT].nStat == (UDWORD)sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_REPAIR:
			if (psDroid->asBits[COMP_REPAIRUNIT].nStat == (UDWORD)sVal.v.ival)
			{
				found = true;
			}
			break;
		case ST_WEAPON:
			for (i=0; i < DROID_MAXWEAPS; i++)
			{
				if (psDroid->asWeaps[i].nStat == (UDWORD)sVal.v.ival)
				{
					found = true;
					break;
				}
			}
			break;
		default:
			ASSERT( false, "scrGetUnit: unknown type" );
			return false;
		}

		if (found)
		{
			break;
		}
	}

	//make sure pass NULL back if not got one
	if (!found)
	{
		psDroid = NULL;
	}

	scrFunctionResult.v.oval = psDroid;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets all the scroll params for the map
WZ_DECL_UNUSED static BOOL scrSetScrollParams(void)
{
	SDWORD		minX, minY, maxX, maxY, prevMinX, prevMinY, prevMaxX, prevMaxY;

	if (!stackPopParams(4, VAL_INT, &minX, VAL_INT, &minY, VAL_INT, &maxX, VAL_INT, &maxY))
	{
		return false;
	}

	// check that the values entered are valid
	ASSERT(minX >= 0, "Minimum scroll x value %d is less than zero - ", minX);
	ASSERT(minY >= 0, "Minimum scroll y value %d is less than zero - ", minY);
	ASSERT(maxX <= mapWidth, "Maximum scroll x value %d is greater than mapWidth %d", maxX, (int)mapWidth);
	ASSERT(maxY <= mapHeight, "Maximum scroll y value %d is greater than mapHeight %d", maxY, (int)mapHeight);

	prevMinX = scrollMinX;
	prevMinY = scrollMinY;
	prevMaxX = scrollMaxX;
	prevMaxY = scrollMaxY;

	scrollMinX = minX;
	scrollMaxX = maxX;
	scrollMinY = minY;
	scrollMaxY = maxY;

	// When the scroll limits change midgame - need to redo the lighting
	initLighting(prevMinX < scrollMinX ? prevMinX : scrollMinX,
				 prevMinY < scrollMinY ? prevMinY : scrollMinY,
				 prevMaxX < scrollMaxX ? prevMaxX : scrollMaxX,
				 prevMaxY < scrollMaxY ? prevMaxY : scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets the scroll minX separately for the map
WZ_DECL_UNUSED static BOOL scrSetScrollMinX(void)
{
	SDWORD				minX, prevMinX;

	if (!stackPopParams(1, VAL_INT, &minX))
	{
		return false;
	}

	//check the value entered are valid
	if (minX < 0)
	{
		ASSERT( false, "Minimum scroll x value %d is less than zero - ", minX );
		return false;
	}

	prevMinX = scrollMinX;

	scrollMinX = minX;

	//when the scroll limits change midgame - need to redo the lighting
	initLighting(prevMinX < scrollMinX ? prevMinX : scrollMinX,
		scrollMinY, scrollMaxX, scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets the scroll minY separately for the map
WZ_DECL_UNUSED static BOOL scrSetScrollMinY(void)
{
	SDWORD				minY, prevMinY;

	if (!stackPopParams(1, VAL_INT, &minY))
	{
		return false;
	}

	//check the value entered are valid
	if (minY < 0)
	{
		ASSERT( false, "Minimum scroll y value %d is less than zero - ", minY );
		return false;
	}

	prevMinY = scrollMinY;

	scrollMinY = minY;

	//when the scroll limits change midgame - need to redo the lighting
	initLighting(scrollMinX,
		prevMinY < scrollMinY ? prevMinY : scrollMinY,
		scrollMaxX, scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets the scroll maxX separately for the map
WZ_DECL_UNUSED static BOOL scrSetScrollMaxX(void)
{
	SDWORD				maxX, prevMaxX;

	if (!stackPopParams(1, VAL_INT, &maxX))
	{
		return false;
	}

	//check the value entered are valid
	if (maxX > (SDWORD)mapWidth)
	{
		ASSERT( false, "Maximum scroll x value %d is greater than mapWidth - ", maxX );
		return false;
	}

	prevMaxX = scrollMaxX;

	scrollMaxX = maxX;

	//when the scroll limits change midgame - need to redo the lighting
	initLighting(scrollMinX,  scrollMinY,
		prevMaxX < scrollMaxX ? prevMaxX : scrollMaxX,
		scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets the scroll maxY separately for the map
WZ_DECL_UNUSED static BOOL scrSetScrollMaxY(void)
{
	SDWORD				maxY, prevMaxY;

	if (!stackPopParams(1, VAL_INT, &maxY))
	{
		return false;
	}

	//check the value entered are valid
	if (maxY > (SDWORD)mapHeight)
	{
		ASSERT( false, "Maximum scroll y value %d is greater than mapWidth - ", maxY );
		return false;
	}

	prevMaxY = scrollMaxY;

	scrollMaxY = maxY;

	//when the scroll limits change midgame - need to redo the lighting
	initLighting(scrollMinX, scrollMinY, scrollMaxX,
		prevMaxY < scrollMaxY ? prevMaxY : scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets which sensor will be used as the default for a player
WZ_DECL_UNUSED static BOOL scrSetDefaultSensor(void)
{
	SDWORD				player;
	UDWORD				sensorInc;

	if (!stackPopParams(2, ST_SENSOR, &sensorInc, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetDefaultSensor:player number is too high" );
		return false;
	}

	//check is a valid sensor Inc
	if (sensorInc > numSensorStats)
	{
		ASSERT( false, "scrSetDefaultSensor: Sensor Inc is too high - %d", sensorInc );
		return false;
	}

	//check that this sensor is a default sensor
	if (asSensorStats[sensorInc].location != LOC_DEFAULT)
	{

		ASSERT( false, "scrSetDefaultSensor: This sensor is not a default one - %s",
			getStatName(&asSensorStats[sensorInc]) );
		return false;
	}

	//assign since OK!
	aDefaultSensor[player] = sensorInc;

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets which ECM will be used as the default for a player
WZ_DECL_UNUSED static BOOL scrSetDefaultECM(void)
{
	SDWORD				player;
	UDWORD				ecmInc;

	if (!stackPopParams(2, ST_ECM, &ecmInc, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetDefaultECM:player number is too high" );
		return false;
	}

	//check is a valid ecmInc
	if (ecmInc > numECMStats)
	{
		ASSERT( false, "scrSetDefaultECM: ECM Inc is too high - %d", ecmInc );
		return false;
	}

	//check that this ecm is a default ecm
	if (asECMStats[ecmInc].location != LOC_DEFAULT)
	{
		ASSERT( false, "scrSetDefaultECM: This ecm is not a default one - %s",
			getStatName(&asECMStats[ecmInc]) );
		return false;
	}

	//assign since OK!
	aDefaultECM[player] = ecmInc;

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets which RepairUnit will be used as the default for a player
WZ_DECL_UNUSED static BOOL scrSetDefaultRepair(void)
{
	SDWORD				player;
	UDWORD				repairInc;

	if (!stackPopParams(2, ST_REPAIR, &repairInc, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetDefaultRepair:player number is too high" );
		return false;
	}

	//check is a valid repairInc
	if (repairInc > numRepairStats)
	{
		ASSERT( false, "scrSetDefaultRepair: Repair Inc is too high - %d", repairInc );
		return false;
	}

	//check that this repair is a default repair
	if (asRepairStats[repairInc].location != LOC_DEFAULT)
	{
		ASSERT( false, "scrSetDefaultRepair: This repair is not a default one - %s",
			getStatName(&asRepairStats[repairInc]) );
		return false;
	}

	//assign since OK!
	aDefaultRepair[player] = repairInc;

	return true;
}

// -----------------------------------------------------------------------------------------
/// Sets the structure limits for a player
static int scrSetStructureLimits(lua_State *L)
{
	STRUCTURE_LIMITS	*psStructLimits;
	
	int index = luaWZObj_checkstructurestat(L, 1);
	int limit = luaL_checkint(L, 2);
	int player = luaWZ_checkplayer(L, 3);

	if (limit < 0)
	{
		return luaL_error(L, "scrSetStructureLimits: limit is less than zero - %d", limit );
	}

	if (limit > LOTS_OF)
	{
		return luaL_error(L, "scrSetStructureLimits: limit is too high - %d - must be less than %d",
			limit, LOTS_OF );
	}

	psStructLimits = asStructLimits[player];
	psStructLimits[index].limit = (UBYTE)limit;

	psStructLimits[index].globalLimit = (UBYTE)limit;

	return 0;
}



// -----------------------------------------------------------------------------------------
/// multiplayer limit handler.
static int scrApplyLimitSet(lua_State *L)
{
	applyLimitSet();
	return 0;
}

// -----------------------------------------------------------------------------------------
/// plays a sound for the specified player - only plays the sound if the
/// specified player = selectedPlayer
static int scrPlaySound(lua_State *L)
{
	int soundID;
	const char *filename = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	soundID = audio_GetTrackID( filename );
	if (soundID == SAMPLE_NOT_FOUND)
	{
		return false;
	}

	if (player == (SDWORD)selectedPlayer)
	{
		audio_QueueTrack(soundID);
		if(bInTutorial)
		{
			audio_QueueTrack(ID_SOUND_OF_SILENCE);
		}
	}
	return 0;
}

// -----------------------------------------------------------------------------------------
/// plays a sound for the specified player - only plays the sound if the
/// specified player = selectedPlayer - saves position
static int scrPlaySoundPos(lua_State *L)
{
	int soundID;
	const char *filename = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	int iX = luaL_checkint(L, 3);
	int iY = luaL_checkint(L, 4);
	int iZ = luaL_checkint(L, 5);
	
	soundID = audio_GetTrackID( filename );
	if (soundID == SAMPLE_NOT_FOUND)
	{
		return false;
	}

	if (player == (SDWORD)selectedPlayer)
	{
		audio_QueueTrackPos(soundID, iX, iY, iZ);
	}
	return 0;
}

// -----------------------------------------------------------------------------------------

/* add a text message to the top of the screen for the selected player*/
static int scrShowConsoleText(lua_State *L)
{
	const char *pText = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	if (player == (SDWORD)selectedPlayer)
	{
		permitNewConsoleMessages(true);
		addConsoleMessage(pText, CENTRE_JUSTIFY, SYSTEM_MESSAGE);
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
/** add a text message to the top of the screen for the selected player*/
static int scrAddConsoleText(lua_State *L)
{
	const char *pText = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	if (player == (SDWORD)selectedPlayer)
	{
		permitNewConsoleMessages(true);
		setConsolePermanence(true,true);
		addConsoleMessage(pText, CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		permitNewConsoleMessages(false);
	}

	return 0;
}



// -----------------------------------------------------------------------------------------
/** add a text message to the top of the screen for the selected player - without clearing whats there*/
static int scrTagConsoleText(lua_State *L)
{
	const char *pText = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	if (player == (SDWORD)selectedPlayer)
	{
		permitNewConsoleMessages(true);
		setConsolePermanence(true,false);
		addConsoleMessage(pText, CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		permitNewConsoleMessages(false);
	}

	return 0;
}

static int scrClearConsole(lua_State *L)
{
	flushConsoleMessages();
	return 0;
}

// -----------------------------------------------------------------------------------------
//demo functions for turning the power on
WZ_DECL_UNUSED static BOOL scrTurnPowerOff(void)
{
	//powerCalculated = false;
	powerCalc(false);

	return true;
}

// -----------------------------------------------------------------------------------------
//demo functions for turning the power off
WZ_DECL_UNUSED static BOOL scrTurnPowerOn(void)
{

	//powerCalculated = true;
	powerCalc(true);

	return true;
}

// -----------------------------------------------------------------------------------------
//flags when the tutorial is over so that console messages can be turned on again
WZ_DECL_UNUSED static BOOL scrTutorialEnd(void)
{
	initConsoleMessages();
	return true;
}

// -----------------------------------------------------------------------------------------
//function to play a full-screen video in the middle of the game for the selected player
WZ_DECL_UNUSED static BOOL scrPlayVideo(void)
{
	char				*pVideo, *pText;

	if (!stackPopParams(2, ST_TEXTSTRING, &pVideo, ST_TEXTSTRING, &pText))
	{
		return false;
	}

		seq_ClearSeqList();
		seq_AddSeqToList(pVideo, NULL, pText, false);		// Arpzzzzzzzzzzzzzzzlksht!
		seq_StartNextFullScreenVideo();

	return true;
}

// -----------------------------------------------------------------------------------------
///checks to see if there are any droids for the specified player
static int scrAnyDroidsLeft(lua_State *L)
{
	BOOL		droidsLeft;

	int player = luaWZ_checkplayer(L, 1);

	//check the players list for any droid
	droidsLeft = true;
	if (apsDroidLists[player] == NULL)
	{
		droidsLeft = false;
	}
	lua_pushboolean(L, droidsLeft);
	return 1;
}

// -----------------------------------------------------------------------------------------
///function to call when the game is over, plays a message then does game over stuff.
static int scrGameOverMessage(lua_State *L)
{
	MESSAGE			*psMessage;
	VIEWDATA		*psViewData;

	const char *message = luaL_checkstring(L, 1);
	MESSAGE_TYPE msgType = luaL_checkint(L, 2);
	int player = luaWZ_checkplayer(L, 3);
	BOOL gameWon = luaL_checkboolean(L, 4);
	
	psViewData = getViewData(message);

	if(gameWon)
	{
		addConsoleMessage(_("YOU ARE VICTORIOUS!"),DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	else
	{
		addConsoleMessage(_("YOU WERE DEFEATED!"),DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}

	//create the message
	psMessage = addMessage(msgType, false, player);

	if (msgType == MSG_PROXIMITY)
	{
	    return luaL_error(L, "Bad message type (MSG_PROXIMITY)" );
	}
	if (psMessage)
	{
		//set the data
		psMessage->pViewData = (MSG_VIEWDATA *)psViewData;
		displayImmediateMessage(psMessage);
		stopReticuleButtonFlash(IDRET_INTEL_MAP);

		//we need to set this here so the VIDEO_QUIT callback is not called
		setScriptWinLoseVideo((UBYTE)(gameWon ? PLAY_WIN : PLAY_LOSE));
	}
	debug(LOG_MSG, "Game over message");

	// this should be called when the video Quit is processed
	// not always is tough, so better be sure
	displayGameOver(gameWon);

	if (challengeActive)
	{
		char sPath[64], key[64], timestr[32], *fStr;
		int seconds = 0, newtime = (gameTime - mission.startTime) / GAME_TICKS_PER_SEC;
		bool victory = false;
		dictionary *dict = iniparser_load(CHALLENGE_SCORES);

		fStr = strrchr(sRequestResult, '/');
		fStr++;	// skip slash
		if (fStr == '\0')
		{
			debug(LOG_ERROR, "Bad path to challenge file (%s)", sRequestResult);
			return true;
		}
		sstrcpy(sPath, fStr);
		sPath[strlen(sPath) - 4] = '\0';	// remove .ini
		if (dict)
		{
			ssprintf(key, "%s:Victory", sPath);
			victory = iniparser_getboolean(dict, key, false);
			ssprintf(key, "%s:seconds", sPath);
			seconds = iniparser_getint(dict, key, 0);
		}
		else
		{
			dict = dictionary_new(3);
		}

		// Update score if we have a victory and best recorded was a loss,
		// or both were losses but time is higher, or both were victories
		// but time is lower.
		if ((!victory && gameWon) 
		    || (!gameWon && !victory && newtime > seconds)
		    || (gameWon && victory && newtime < seconds))
		{
			dictionary_set(dict, sPath, NULL);
			ssprintf(key, "%s:Seconds", sPath);
			ssprintf(timestr, "%d", newtime);
			iniparser_setstring(dict, key, timestr);
			ssprintf(key, "%s:Player", sPath);
			iniparser_setstring(dict, key, NetPlay.players[selectedPlayer].name);
			ssprintf(key, "%s:Victory", sPath);
			iniparser_setstring(dict, key, gameWon ? "true": "false");
		}
		iniparser_dump_ini(dict, CHALLENGE_SCORES);
		iniparser_freedict(dict);
	}

	return 0;
}


// -----------------------------------------------------------------------------------------
//function to call when the game is over
WZ_DECL_UNUSED static BOOL scrGameOver(void)
{
	BOOL	gameOver;

	if (!stackPopParams(1, VAL_BOOL, &gameOver))
	{
		return false;
	}

    /*this function will only be called with gameOver = true when at the end of
    the game so we'll just hard-code what happens!*/

	//don't want this in multiplayer...
	if (!bMultiPlayer)

    {
        if (gameOver == true && !bInTutorial)
        {
            //we need to set this here so the VIDEO_QUIT callback is not called
		    setScriptWinLoseVideo(PLAY_WIN);

			seq_ClearSeqList();

	        seq_AddSeqToList("outro.ogg", NULL, "outro.txa", false);
	        seq_StartNextFullScreenVideo();

		}
	}

	return true;
}

// -----------------------------------------------------------------------------------------
static int scrAnyFactoriesLeft(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	STRUCTURE *psCurr;

	//check the players list for any structures
	if(apsStructLists[player])
	{
		for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if(StructIsFactory(psCurr))
			{
				lua_pushboolean(L, true);
				return 1;
			}
		}
	}

	return 0;
}


// -----------------------------------------------------------------------------------------
///checks to see if there are any structures (except walls) for the specified player
static int scrAnyStructButWallsLeft(lua_State *L)
{
	BOOL		structuresLeft;
	STRUCTURE	*psCurr;
	
	int player = luaWZ_checkplayer(L, 1);

	//check the players list for any structures
	structuresLeft = true;
	if (apsStructLists[player] == NULL)
	{
		structuresLeft = false;
	}
	else
	{
		structuresLeft = false;
		for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if (psCurr->pStructureType->type != REF_WALL && psCurr->pStructureType->
				type != REF_WALLCORNER)
			{
				structuresLeft = true;
				break;
			}
		}
	}

	lua_pushboolean(L, structuresLeft);
	return 1;
}

// -----------------------------------------------------------------------------------------
//defines the background audio to play
WZ_DECL_UNUSED static BOOL scrPlayBackgroundAudio(void)
{
	char	*pText;
	SDWORD	iVol;

	if (!stackPopParams(2, ST_TEXTSTRING, &pText, VAL_INT, &iVol))
	{
		return false;
	}


	audio_PlayStream(pText, (float)iVol / 100.f, NULL, NULL);


	return true;

}

static int scrPlayIngameCDAudio(lua_State *L)
{
	debug(LOG_SOUND, "Script wanted music to start");
	cdAudio_PlayTrack(SONG_INGAME);

	return 0;
}

// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL scrStopCDAudio(void)
{
	debug(LOG_SOUND, "Script wanted music to stop");
	cdAudio_Stop();
	return true;
}

// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL scrPauseCDAudio(void)
{
	cdAudio_Pause();
	return true;
}

// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL scrResumeCDAudio(void)
{
	cdAudio_Resume();
	return true;
}

// -----------------------------------------------------------------------------------------
// set the retreat point for a player
static int scrSetRetreatPoint(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	Vector2i coord = luaWZ_checkWorldCoords(L, 2);

	asRunData[player].sPos.x = coord.x;
	asRunData[player].sPos.y = coord.y;

	return 0;
}

// -----------------------------------------------------------------------------------------
// set the retreat force level
static int scrSetRetreatForce(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int level = luaL_checkint(L, 2);
	int numDroids;
	DROID *psCurr;

	if (level > 100 || level < 0)
	{
		luaL_error(L, "Level out of range");
	}

	// count up the current number of droids
	numDroids = 0;
	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		numDroids += 1;
	}

	asRunData[player].forceLevel = (UBYTE)(level * numDroids / 100);

	return 0;
}

// -----------------------------------------------------------------------------------------
// set the retreat leadership
static int scrSetRetreatLeadership(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int level = luaL_checkint(L, 2);

	if (level > 100 || level < 0)
	{
		luaL_error(L, "Level out of range");
	}

	asRunData[player].leadership = (UBYTE)level;

	return 0;
}

// -----------------------------------------------------------------------------------------
/// set the retreat point for a group
static int scrSetGroupRetreatPoint(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);

	if (x < 0 || x >= (SDWORD)mapWidth*TILE_UNITS ||
		y < 0 || y >= (SDWORD)mapHeight*TILE_UNITS)
	{
		return luaL_error(L, "scrSetRetreatPoint: coords off map" );
	}

	psGroup->sRunData.sPos.x = x;
	psGroup->sRunData.sPos.y = y;

	return 0;
}

// -----------------------------------------------------------------------------------------
static int scrSetGroupRetreatForce(lua_State *L)
{
	SDWORD numDroids;
	DROID *psCurr;
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	int level = luaL_checkint(L, 2);

	if (level > 100 || level < 0)
	{
		return luaL_error(L, "scrSetRetreatForce: level out of range" );
	}

	// count up the current number of droids
	numDroids = 0;
	for(psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		numDroids += 1;
	}

	psGroup->sRunData.forceLevel = (UBYTE)(level * numDroids / 100);

	return 0;
}

// -----------------------------------------------------------------------------------------
// set the retreat health level
WZ_DECL_UNUSED static BOOL scrSetRetreatHealth(void)
{
	SDWORD	player, health;

	if (!stackPopParams(2, VAL_INT, &player, VAL_INT, &health))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetHealthForce: player out of range" );
		return false;
	}

	if (health > 100 || health < 0)
	{
		ASSERT( false, "scrSetHealthForce: health out of range" );
		return false;
	}

	asRunData[player].healthLevel = (UBYTE)health;

	return true;
}

// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL scrSetGroupRetreatHealth(void)
{
	SDWORD		health;
	DROID_GROUP	*psGroup;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &health))
	{
		return false;
	}

	if (health > 100 || health < 0)
	{
		ASSERT( false, "scrSetGroupRetreatHealth: health out of range" );
		return false;
	}

	psGroup->sRunData.healthLevel = (UBYTE)health;

	return true;
}

// -----------------------------------------------------------------------------------------
/// set the retreat leadership
static int scrSetGroupRetreatLeadership(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	int level = luaL_checkint(L, 2);

	if (level > 100 || level < 0)
	{
		return luaL_error(L, "scrSetRetreatLeadership: level out of range" );
	}

	psGroup->sRunData.leadership = (UBYTE)level;

	return 0;
}

// -----------------------------------------------------------------------------------------
/// start a Mission - the missionType is ignored now - gets it from the level data ***********
static int scrStartMission(lua_State *L)
{
	int missionType = luaL_checkint(L, 1);
	const char *pGame = luaL_checkstring(L, 2);
	LEVEL_DATASET		*psNewLevel;

	//if (missionType > MISSION_NONE)
	if (missionType > LDS_NONE)
	{
		return luaL_error(L, "Invalid Mission Type" );
	}

	// tell the loop that a new level has to be loaded up - not yet!
	//loopNewLevel = true;
	sstrcpy(aLevelName, pGame);

	// find the level dataset
	psNewLevel = levFindDataSet(pGame);
	if (psNewLevel == NULL)
	{
		debug( LOG_FATAL, "scrStartMission: couldn't find level data" );
		abort();
		return luaL_error(L, "couldn't find level data" );
	}

	//set the mission rolling...
	//nextMissionType = missionType;
	nextMissionType = psNewLevel->type;
//	loopMissionState = LMS_SETUPMISSION;
	loopMissionState = LMS_CLEAROBJECTS;

/*	if (!setUpMission(missionType))
	{
		ASSERT( false, "Unable to start mission - %s", pGame );
		return false;
	}*/

	return 0;
}

// -----------------------------------------------------------------------------------------
//set Snow (enable disable snow)
WZ_DECL_UNUSED static BOOL scrSetSnow(void)
{
	BOOL bState;

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}


	if(bState)
	{
		atmosSetWeatherType(WT_SNOWING);
	}
	else
	{
		atmosSetWeatherType(WT_NONE);
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//set Rain (enable disable Rain)
WZ_DECL_UNUSED static BOOL scrSetRain(void)
{
	BOOL bState;

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}


	if(bState)
	{
		atmosSetWeatherType(WT_RAINING);
	}
	else
	{
		atmosSetWeatherType(WT_NONE);
	}

	return true;
}

// -----------------------------------------------------------------------------------------
static int scrSetBackgroundFog(lua_State *L)
{
	BOOL bState = luaL_checkboolean(L, 1);

	//jps 17 feb 99 just set the status let other code worry about fogEnable/reveal
	if (bState)//true, so go to false
	{
		//restart fog if it was off
		if ((fogStatus == 0) && war_GetFog() && !(bMultiPlayer && game.fog))
		{
			pie_EnableFog(true);
		}
		fogStatus |= FOG_BACKGROUND;//set lowest bit of 3
	}
	else
	{
		fogStatus &= FOG_FLAGS-FOG_BACKGROUND;//clear middle bit of 3
		//disable fog if it longer used
		if (fogStatus == 0)
		{
			pie_SetFogStatus(false);
			pie_EnableFog(false);
		}
	}

/* jps 17 feb 99
	if(getRevealStatus())		// fog'o war enabled
	{
		pie_SetFogStatus(false);
		pie_EnableFog(false);
//		fogStatus = 0;
		return true;
	}

	if (bState)//true, so go to false
	{
		if (war_GetFog())
		{
			//restart fog if it was off
			if (fogStatus == 0)
			{
				pie_EnableFog(true);
			}
			fogStatus |= FOG_BACKGROUND;//set lowest bit of 3
		}
	}
	else
	{
		if (war_GetFog())
		{
			fogStatus &= FOG_FLAGS-FOG_BACKGROUND;//clear middle bit of 3
			//disable fog if it longer used
			if (fogStatus == 0)
			{
				pie_SetFogStatus(false);
				pie_EnableFog(false);
			}
		}
	}
*/

	return 0;
}

// -----------------------------------------------------------------------------------------
//set Depth Fog (gradual fog from mid range to edge of world)
static int scrSetDepthFog(lua_State *L)
{
	BOOL bState = luaL_checkboolean(L, 1);

	// ffs am
//jps 17 feb 99 just set the status let other code worry about fogEnable/reveal
	if (bState)//true, so go to false
	{
		//restart fog if it was off
		if ((fogStatus == 0) && war_GetFog() )
		{
			pie_EnableFog(true);
		}
		fogStatus |= FOG_DISTANCE;//set lowest bit of 3
	}
	else
	{
		fogStatus &= FOG_FLAGS-FOG_DISTANCE;//clear middle bit of 3
		//disable fog if it longer used
		if (fogStatus == 0)
		{
			pie_SetFogStatus(false);
			pie_EnableFog(false);
		}
	}

/* jps 17 feb 99	if(getRevealStatus())		// fog'o war enabled
	{
		pie_SetFogStatus(false);
		pie_EnableFog(false);
//		fogStatus = 0;
		return true;
	}

	if (bState)//true, so go to false
	{
		if (war_GetFog())
		{
			//restart fog if it was off
			if (fogStatus == 0)
			{
				pie_EnableFog(true);
			}
			fogStatus |= FOG_DISTANCE;//set lowest bit of 3
		}
	}
	else
	{
		if (war_GetFog())
		{
			fogStatus &= FOG_FLAGS-FOG_DISTANCE;//clear middle bit of 3
			//disable fog if it longer used
			if (fogStatus == 0)
			{
				pie_SetFogStatus(false);
				pie_EnableFog(false);
			}
		}
	}
*/

	return 0;
}

// -----------------------------------------------------------------------------------------
/// set Mission Fog colour, may be modified by weather effects
static int scrSetFogColour(lua_State *L)
{
	PIELIGHT scrFogColour;
	
	int red   = luaL_checkint(L, 1);
	int green = luaL_checkint(L, 2);
	int blue  = luaL_checkint(L, 3);

	if (war_GetFog())
	{
		scrFogColour.byte.r = red;
		scrFogColour.byte.g = green;
		scrFogColour.byte.b = blue;
		scrFogColour.byte.a = 255;

		pie_SetFogColour(scrFogColour);
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
// test function to test variable references
WZ_DECL_UNUSED static BOOL scrRefTest(void)
{
	SDWORD		Num = 0;

	if (!stackPopParams(1,VAL_INT, Num))
	{
		return false;
	}

	debug( LOG_NEVER, "scrRefTest: num: %d \n", Num );

	return true;
}

// -----------------------------------------------------------------------------------------
/// is player a human or computer player? (multiplayer only)
static int scrIsHumanPlayer(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);

	lua_pushboolean(L, isHumanPlayer(player));
	return 1;
}


// -----------------------------------------------------------------------------------------
/// Set an alliance between two players
static int scrCreateAlliance(lua_State *L)
{
	int player1 = luaWZ_checkplayer(L, 1);
	int player2 = luaWZ_checkplayer(L, 2);

	if(bMultiPlayer)
	{
		if(game.alliance==NO_ALLIANCES || game.alliance==ALLIANCES_TEAMS
			|| player1 >= game.maxPlayers || player2>=game.maxPlayers)
		{
			return 0;
		}
	}

	formAlliance((UBYTE)player1, (UBYTE)player2,true,false,true);

	lua_pushboolean(L, true);
	return 1;
}

// -----------------------------------------------------------------------------------------
// offer an alliance
WZ_DECL_UNUSED static BOOL scrOfferAlliance(void)
{
	SDWORD	player1,player2;
	if (!stackPopParams(2, VAL_INT, &player1, VAL_INT, &player2))
	{
		return false;
	}
	if (game.alliance==NO_ALLIANCES || game.alliance==ALLIANCES_TEAMS ||
		player1 < 0 || player1 >= MAX_PLAYERS ||
		player2 < 0 || player2 >= MAX_PLAYERS)
	{
		ASSERT( false, "scrOfferAlliance: player out of range p1=%d p2=%d", player1, player2 );
		return false;
	}


	requestAlliance((UBYTE)player1,(UBYTE)player2,true,true);
	return true;
}


// -----------------------------------------------------------------------------------------
// Break an alliance between two players
WZ_DECL_UNUSED static BOOL scrBreakAlliance(void)
{
	SDWORD	player1,player2;

	if (!stackPopParams(2, VAL_INT, &player1, VAL_INT, &player2))
	{
		return false;
	}

	if (
		player1 < 0 || player1 >= MAX_PLAYERS ||
		player2 < 0 || player2 >= MAX_PLAYERS)
	{
		ASSERT( false, "scrBreakAlliance: player out of range p1=%d p2=%d", player1, player2 );
		return false;
	}
/*
if(bMultiPlayer)
	{


		if(alliances[player1][player2] != ALLIANCE_BROKEN)
		{
			CONPRINTF(ConsoleString,(ConsoleString,"%d and %d break alliance.",player1,player2));
			sendAlliance((UBYTE)player1,(UBYTE)player2,ALLIANCE_BROKEN,0);
		}
}
*/


	if(bMultiPlayer)
	{
		if(game.alliance==NO_ALLIANCES || game.alliance==ALLIANCES_TEAMS
			|| player1 >= game.maxPlayers || player2>=game.maxPlayers)
		{
			return true;
		}
		breakAlliance(player1,player2,true,true);
	}
	else
	{
		breakAlliance(player1,player2,false,true);
	}
/*
	alliances[player1][player2] = ALLIANCE_BROKEN;
	alliances[player2][player1] = ALLIANCE_BROKEN;
*/
	return true;
}


// -----------------------------------------------------------------------------------------
// Multiplayer relevant scriptfuncs
// returns true if 2 or more players are in alliance.
WZ_DECL_UNUSED static BOOL scrAllianceExists(void)
{

	UDWORD i,j;
	for(i=0;i<MAX_PLAYERS;i++)
	{
		for(j=0;j<MAX_PLAYERS;j++)
		{
			if(alliances[i][j] == ALLIANCE_FORMED)
			{
				scrFunctionResult.v.bval = true;
				if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
				{
					return false;
				}
				return true;
			}
		}
	}

	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

static int scrAllianceExistsBetween(lua_State *L)
{
	int i = luaWZ_checkplayer(L, 1);
	int j = luaWZ_checkplayer(L, 2);

	lua_pushboolean(L, alliances[i][j] == ALLIANCE_FORMED);
	return 1;
}

// -----------------------------------------------------------------------------------------
static int scrPlayerInAlliance(lua_State *L)
{
	int j;
	int player = luaWZ_checkplayer(L, 1);

	for(j=0;j<MAX_PLAYERS;j++)
	{
		if(alliances[player][j] == ALLIANCE_FORMED)
		{
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

// -----------------------------------------------------------------------------------------
// returns true if a single alliance is dominant.
WZ_DECL_UNUSED static BOOL scrDominatingAlliance(void)
{
	UDWORD i,j;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		for(j=0;j<MAX_PLAYERS;j++)
		{
			if(   isHumanPlayer(j)
			   && isHumanPlayer(i)
			   && i != j
			   && alliances[i][j] != ALLIANCE_FORMED)
			{
				scrFunctionResult.v.bval = false;
				if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
				{
					return false;
				}
				return true;
			}
		}
// -----------------------------------------------------------------------------------------
	}


	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/// Is the ai responsible for this player?
static int scrMyResponsibility(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);

	lua_pushboolean(L, myResponsibility(player));
	return 1;
}

// -----------------------------------------------------------------------------------------
/** 
 * Checks to see if a structure of the type specified exists within the specified range of an XY location
 * Use player -1 to find structures owned by any player. In this case, ignore if they are completed.
 */
static int scrStructureBuiltInRange(lua_State *L)
{
	int rangeSquared;
	STRUCTURE *psCurr;
	int xdiff, ydiff;
	STRUCTURE_STATS *psTarget;
	
	int index = luaWZObj_checkstructurestat(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);
	int range = luaL_checkint(L, 4);
	int player = luaL_checkint(L, 5);	// player, but may be -1

	if (x < 0
	 || map_coord(x) > (SDWORD)mapWidth)
	{
		return luaL_error(L, "The X coordinate is off map");
	}
	if (y < 0
	 || map_coord(y) > (SDWORD)mapHeight)
	{
		return luaL_error(L, "The Y coordinate is off map");
	}
	if (range < (SDWORD)0)
	{
		return luaL_error(L, "Negative range");
	}

	//now look through the players list of structures to see if this type
	//exists within range
	psTarget = &asStructureStats[index];
	rangeSquared = range * range;
	for(psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		xdiff = (SDWORD)psCurr->pos.x - x;
		ydiff = (SDWORD)psCurr->pos.y - y;
		if (xdiff*xdiff + ydiff*ydiff <= rangeSquared)
		{

			if( strcmp(psCurr->pStructureType->pName,psTarget->pName) == 0 )
			{
				if (psCurr->status == SS_BUILT)
				{
					luaWZObj_pushstructure(L, psCurr);
					return 1;
				}
			}
		}
	}
	lua_pushnil(L);
	return 1;
}


// -----------------------------------------------------------------------------------------
// generate a random number
WZ_DECL_UNUSED static BOOL scrRandom(void)
{
	SDWORD		range, iResult;

	if (!stackPopParams(1, VAL_INT, &range))
	{
		return false;
	}

	if (range == 0)
	{
		iResult = 0;
	}
	else
	{
		iResult = rand()%abs(range);
	}

	scrFunctionResult.v.ival = iResult;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// randomise the random number seed
static int scrRandomiseSeed(lua_State *L)
{
	// Why? What's the point? What on earth were they thinking, exactly? If the numbers don't have enough randominess, just set the random seed again and again until the numbers are double-plus super-duper full of randonomium?
	debug(LOG_ERROR, "A script is trying to set the random seed with srand(). That just doesn't make sense.");
	//srand((UDWORD)clock());

	return 0;
}

// -----------------------------------------------------------------------------------------
//explicitly enables a research topic
static int scrEnableResearch(lua_State *L)
{
	RESEARCH	*psResearch;
	
	const char *researchName = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	psResearch = getResearch(researchName, false);
	if (!psResearch)
	{
		return luaL_error(L, "could not find research");
	}

	enableResearch(psResearch, player);
	
	return 0;
}

// -----------------------------------------------------------------------------------------
//acts as if the research topic was completed - used to jump into the tree
static int scrCompleteResearch(lua_State *L)
{
	unsigned int researchIndex;
	
	const char *researchName = luaL_checkstring(L, 1);
	RESEARCH *psResearch = getResearch(researchName, false);
	int player = luaWZ_checkplayer(L, 2);

	if(psResearch == NULL)
	{
		return luaL_error(L, "scrCompleteResearch: no such research topic" );
	}

	researchIndex = psResearch - asResearch;	//TODO: fix if needed
	if (researchIndex > numResearch)
	{
		return luaL_error(L, "scrCompleteResearch: invalid research index" );
	}

	if(bMultiMessages && (gameTime > 2 ))
	{
		SendResearch(player, researchIndex, false);
		// Wait for our message before doing anything.
	}
	else
	{
		researchResult(researchIndex, player, false, NULL, false);
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
// This routine used to start just a reticule button flashing
//   .. now it starts any button flashing (awaiting implmentation from widget library)
static int scrFlashOn(lua_State *L)
{
	int button = luaL_checkint(L, 1);

	// For the time being ... we will perform the old code for the reticule ...
	if (button >= IDRET_OPTIONS && button <= IDRET_CANCEL)
	{
		flashReticuleButton((UDWORD)button);
		return 0;
	}

	if(widgGetFromID(psWScreen,button) != NULL)
	{
		widgSetButtonFlash(psWScreen,button);
	}
	return 0;
}


// -----------------------------------------------------------------------------------------
// stop a generic button flashing
static int scrFlashOff(lua_State *L)
{
	int button = luaL_checkint(L, 1);

	if (button >= IDRET_OPTIONS && button <= IDRET_CANCEL)
	{
		stopReticuleButtonFlash((UDWORD)button);
		return 0;
	}

	if(widgGetFromID(psWScreen,button) != NULL)
	{
		widgClearButtonFlash(psWScreen,button);
	}
	return 0;
}

// -----------------------------------------------------------------------------------------
//set the initial power level settings for a player
static int scrSetPowerLevel(lua_State *L)
{
	int player, power;
	power = luaL_checkint(L, 1);
	player = luaWZ_checkplayer(L, 2);

	setPower(player, power);

	return 0;
}

// -----------------------------------------------------------------------------------------
//add some power for a player
static int scrAddPower(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	float power = luaL_checknumber(L, 2);
	giftPower(ANYPLAYER, player, power, true);
	return 0;
}

// -----------------------------------------------------------------------------------------
/*set the landing Zone position for the map - this is for player 0. Can be
scrapped and replaced by setNoGoAreas, left in for compatibility*/
WZ_DECL_UNUSED static BOOL scrSetLandingZone(void)
{
	SDWORD		x1, x2, y1, y2;

	if (!stackPopParams(4, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	//check the values - check against max possible since can set in one mission for the next
	//if (x1 > (SDWORD)mapWidth)
	if (x1 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLandingZone: x1 is greater than max mapWidth" );
		return false;
	}
	//if (x2 > (SDWORD)mapWidth)
	if (x2 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLandingZone: x2 is greater than max mapWidth" );
		return false;
	}
	//if (y1 > (SDWORD)mapHeight)
	if (y1 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetLandingZone: y1 is greater than max mapHeight" );
		return false;
	}
	//if (y2 > (SDWORD)mapHeight)
	if (y2 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetLandingZone: y2 is greater than max mapHeight" );
		return false;
	}
	//check won't overflow!
	if (x1 > UBYTE_MAX || y1 > UBYTE_MAX || x2 > UBYTE_MAX || y2 > UBYTE_MAX)
	{
		ASSERT( false, "scrSetLandingZone: one coord is greater than %d", UBYTE_MAX );
		return false;
	}

	setLandingZone((UBYTE)x1, (UBYTE)y1, (UBYTE)x2, (UBYTE)y2);

	return true;
}

/*set the landing Zone position for the Limbo droids and adds the Limbo droids
to the world at the location*/
WZ_DECL_UNUSED static BOOL scrSetLimboLanding(void)
{
	SDWORD		x1, x2, y1, y2;

	if (!stackPopParams(4, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	//check the values - check against max possible since can set in one mission for the next
	//if (x1 > (SDWORD)mapWidth)
	if (x1 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLimboLanding: x1 is greater than max mapWidth" );
		return false;
	}
	//if (x2 > (SDWORD)mapWidth)
	if (x2 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLimboLanding: x2 is greater than max mapWidth" );
		return false;
	}
	//if (y1 > (SDWORD)mapHeight)
	if (y1 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetLimboLanding: y1 is greater than max mapHeight" );
		return false;
	}
	//if (y2 > (SDWORD)mapHeight)
	if (y2 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetLimboLanding: y2 is greater than max mapHeight" );
		return false;
	}
	//check won't overflow!
	if (x1 > UBYTE_MAX || y1 > UBYTE_MAX || x2 > UBYTE_MAX || y2 > UBYTE_MAX)
	{
		ASSERT( false, "scrSetLimboLanding: one coord is greater than %d", UBYTE_MAX );
		return false;
	}

	setNoGoArea((UBYTE)x1, (UBYTE)y1, (UBYTE)x2, (UBYTE)y2, LIMBO_LANDING);

	//this calls the Droids from the Limbo list onto the map
	placeLimboDroids();

	return true;
}

// -----------------------------------------------------------------------------------------
//initialises all the no go areas
static int scrInitAllNoGoAreas(lua_State *L)
{
	initNoGoAreas();

	return 0;
}

// -----------------------------------------------------------------------------------------
//set a no go area for the map - landing zones for the enemy, or player 0
static int scrSetNoGoArea(lua_State *L)
{
	int x1 = luaL_checkint(L, 1);
	int y1 = luaL_checkint(L, 2);
	int x2 = luaL_checkint(L, 3);
	int y2 = luaL_checkint(L, 4);
	int area = luaL_checkint(L, 5);

	if (area == LIMBO_LANDING)
	{
		return luaL_error(L, "Cannot set the Limbo Landing area with this function" );
	}

	//check the values - check against max possible since can set in one mission for the next
	//if (x1 > (SDWORD)mapWidth)
	if (x1 > (SDWORD)MAP_MAXWIDTH)
	{
		return luaL_error(L, "x1 is greater than max mapWidth" );
	}
	//if (x2 > (SDWORD)mapWidth)
	if (x2 > (SDWORD)MAP_MAXWIDTH)
	{
		return luaL_error(L, "x2 is greater than max mapWidth" );
	}
	//if (y1 > (SDWORD)mapHeight)
	if (y1 > (SDWORD)MAP_MAXHEIGHT)
	{
		return luaL_error(L, "y1 is greater than max mapHeight" );
	}
	//if (y2 > (SDWORD)mapHeight)
	if (y2 > (SDWORD)MAP_MAXHEIGHT)
	{
		return luaL_error(L, "y2 is greater than max mapHeight" );
		return false;
	}
	//check won't overflow!
	if (x1 > UBYTE_MAX || y1 > UBYTE_MAX || x2 > UBYTE_MAX || y2 > UBYTE_MAX)
	{
		return luaL_error(L, "one coord is greater than %d", UBYTE_MAX );
	}

	if (area >= MAX_NOGO_AREAS)
	{
		return luaL_error(L, "max num of areas is %d", MAX_NOGO_AREAS );
	}

	setNoGoArea((UBYTE)x1, (UBYTE)y1, (UBYTE)x2, (UBYTE)y2, (UBYTE)area);

	return 0;
}


// -----------------------------------------------------------------------------------------
/// set the zoom level for the radar
/// What is the script doing setting radar zoom? Commenting out for now. - Per
static int scrSetRadarZoom(lua_State *L)
{
#if 0
	int level = luaL_checkint(L,1);

	if (level < 0 || level > 2)
	{
		return luaL_error(L, "scrSetRadarZoom: zoom level out of range" );
	}

	SetRadarZoom((UWORD)level);
#endif
	return 0;
}

// -----------------------------------------------------------------------------------------
//set how long an offworld mission can last -1 = no limit
static int scrSetMissionTime(lua_State *L)
{
	double dtime = luaL_checknumber(L, 1);
	int time = dtime*1000;

	//check not more than one hour - the mission timers cannot cope at present! - (visually)
	//if (time > 60*60*GAME_TICKS_PER_SEC)
	//check not more than 99 mins - the mission timers cannot cope at present! - (visually)
	//we're allowing up to 5 hours now!
	if (time > 5*60*60*GAME_TICKS_PER_SEC)
	{
		ASSERT( false,"The mission timer cannot be set to more than 99!" );
		time = -1;
	}
	//store the value
	mission.time = time;
		// ffs ab	... but shouldn't this be on the psx ?
	setMissionCountDown();


	//add the timer to the interface
	if (mission.time >= 0)
	{
		mission.startTime = gameTime;
		addMissionTimerInterface();
	}
	else
	{
		//make sure its not up if setting to -1
		intRemoveMissionTimer();
		//make sure the cheat time is not set
		mission.cheatTime = 0;
	}

	return 0;
}

// this returns how long is left for the current mission time is 1/100th sec - same units as passed in
WZ_DECL_UNUSED static BOOL scrMissionTimeRemaining(void)
{
	SDWORD	  timeRemaining;

	timeRemaining = mission.time - (gameTime - mission.startTime);

	if (timeRemaining < 0)
	{
		timeRemaining = 0;
	}
	else
	{
		timeRemaining /= 100;
	}

	scrFunctionResult.v.ival = timeRemaining;
	if(!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return(false);
	}
	return(true);
}

// -----------------------------------------------------------------------------------------
/// set the time delay for reinforcements for an offworld mission
static int scrSetReinforcementTime(lua_State *L)
{
	DROID	   *psDroid;

	double dtime = luaL_checknumber(L, 1);
	int time = dtime*1000;

	//check not more than one hour - the mission timers cannot cope at present!
	if (time != LZ_COMPROMISED_TIME && time > 60*60*GAME_TICKS_PER_SEC)
	{
		ASSERT( false,"The transport timer cannot be set to more than 1 hour!" );
		time = -1;
	}

	//not interseted in this check any more -  AB 28/01/99
	//quick check of the value - don't check if time has not been set
	/*if (mission.time > 0 && time != LZ_COMPROMISED_TIME && time > mission.time)
	{
		DBMB(("scrSetReinforcementTime: reinforcement time greater than mission time!"));
	}*/
	//store the value
	mission.ETA = time;

	//if offworld or campaign change mission, then add the timer
	//if (mission.type == LDS_MKEEP || mission.type == LDS_MCLEAR ||
	//	mission.type == LDS_CAMCHANGE)
	if (missionCanReEnforce())
	{
		addTransporterTimerInterface();
	}

	//make sure the timer is not there if the reinforcement time has been set to < 0
	if (time < 0)
	{

		intRemoveTransporterTimer();

		/*only remove the launch if haven't got a transporter droid since the
		scripts set the time to -1 at the between stage if there are not going
		to be reinforcements on the submap  */
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid =
			psDroid->psNext)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				break;
			}
		}
		//if not found a transporter, can remove the launch button
		if (psDroid ==  NULL)
		{
			intRemoveTransporterLaunch();
		}
	}

	return 0;
}

// -----------------------------------------------------------------------------------------
// Sets all structure limits for a player to a specified value
WZ_DECL_UNUSED static BOOL scrSetAllStructureLimits(void)
{
	SDWORD				player, limit;
	STRUCTURE_LIMITS	*psStructLimits;
	UDWORD				i;

	if (!stackPopParams(2, VAL_INT, &limit, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetStructureLimits:player number is too high" );
		return false;
	}

	if (limit < 0)
	{
		ASSERT( false, "scrSetStructureLimits: limit is less than zero - %d", limit );
		return false;
	}

	if (limit > LOTS_OF)
	{
		ASSERT( false, "scrSetStructureLimits: limit is too high - %d - must be less than %d",
			limit, LOTS_OF );
		return false;
	}

	//set all the limits to the value specified
	psStructLimits = asStructLimits[player];
	for (i = 0; i < numStructureStats; i++)
	{
		psStructLimits[i].limit = (UBYTE)limit;

		psStructLimits[i].globalLimit = (UBYTE)limit;

	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Establishes the distance between two points - uses an approximation
WZ_DECL_UNUSED static BOOL scrDistanceTwoPts( void )
{
	SDWORD	x1,y1,x2,y2;

	if(!stackPopParams(4,VAL_INT,&x1,VAL_INT,&y1,VAL_INT,&x2,VAL_INT,&y2))
	{
		ASSERT( false,"SCRIPT : Distance between two points - cannot get parameters" );
		return(false);
	}

	/* Approximate the distance */
	scrFunctionResult.v.ival = hypotf(x1 - x2, y1 - y2);
	if(!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		ASSERT( false,"SCRIPT : Distance between two points - cannot return scrFunctionResult" );
		return(false);
	}
	return(true);
}

// -----------------------------------------------------------------------------------------
// Returns whether two objects can see each other
WZ_DECL_UNUSED static BOOL	scrLOSTwoBaseObjects( void )
{
BASE_OBJECT	*psSource,*psDest;
BOOL		bWallsBlock;
BOOL		retVal;

	if(!stackPopParams(3,ST_BASEOBJECT,&psSource,ST_BASEOBJECT,&psDest,VAL_BOOL,&bWallsBlock))
	{
		ASSERT( false,"SCRIPT : scrLOSTwoBaseObjects - cannot get parameters" );
		return(false);
	}

	retVal = visibleObject(psSource, psDest, bWallsBlock);

	scrFunctionResult.v.bval = retVal;
	if(!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		ASSERT( false,"SCRIPT : scrLOSTwoBaseObjects - cannot return scrFunctionResult" );
		return(false);
	}
	return(true);
}

// -----------------------------------------------------------------------------------------
/// Destroys all structures within a certain bounding area.
static int scrKillStructsInArea(lua_State *L)
{
	STRUCTURE	*psStructure,*psNextS;
	FEATURE		*psFeature,*psNextF;
	SDWORD		sX,sY;
	
	int player = luaWZ_checkplayer(L, 1);
	unsigned int typeRef = luaL_checkint(L, 2);
	int x1 = luaL_checkint(L, 3);
	int y1 = luaL_checkint(L, 4);
	int x2 = luaL_checkint(L, 5);
	int y2 = luaL_checkint(L, 6);
	BOOL bVisible = luaL_checkboolean(L, 7);
	BOOL bTakeFeatures = luaL_checkboolean(L, 8);

	for(psStructure = apsStructLists[player]; psStructure; psStructure = psNextS)
	{
		/* Keep a copy */
		psNextS = psStructure->psNext;

		sX = psStructure->pos.x;
		sY = psStructure->pos.y;

		if(psStructure->pStructureType->type == typeRef)
		{
			if(sX >= x1 && sX <=x2 && sY >= y1 && sY <= y2)
			{
				if(bVisible)
				{
					SendDestroyStructure(psStructure);
				}
				else
				{
					removeStruct(psStructure, true);
				}
			}
		}
	}

	if(bTakeFeatures)
	{
		for(psFeature = apsFeatureLists[0]; psFeature; psFeature = psNextF)
		{
			/* Keep a copy */
			psNextF = psFeature->psNext;

			sX = psFeature->pos.x;
			sY = psFeature->pos.y;

		  	if( psFeature->psStats->subType == FEAT_BUILDING)
		  //		(psFeature->psStats->subType != FEAT_OIL_DRUM) &&
		  //		(psFeature->psStats->subType != FEAT_OIL_RESOURCE) )

			{
				if(sX >= x1 && sX <=x2 && sY >= y1 && sY <= y2)
				{
					if(bVisible)
					{
						destroyFeature(psFeature);
					}
					else
					{
						removeFeature(psFeature);
					}
				}
			}
		}
	}
	return 0;
}
// -----------------------------------------------------------------------------------------
// Returns a value representing the threat from droids in a given area
WZ_DECL_UNUSED static BOOL	scrThreatInArea( void )
{
SDWORD	x1,y1,x2,y2;
SDWORD	ldThreat,mdThreat,hdThreat;
UDWORD	playerLooking,playerTarget;
SDWORD	totalThreat;
DROID	*psDroid;
SDWORD	dX,dY;
BOOL	bVisible;

	if(!stackPopParams(10,VAL_INT,&playerLooking,VAL_INT,&playerTarget,VAL_INT,&x1,VAL_INT,&y1,VAL_INT,&x2,VAL_INT,&y2,
		VAL_INT,&ldThreat,VAL_INT,&mdThreat,VAL_INT,&hdThreat, VAL_BOOL, &bVisible))
	{
		ASSERT( false,"SCRIPT : scrThreatInArea - Cannot get parameters" );
		return(false);
	}

	if(playerLooking>=MAX_PLAYERS || playerTarget >= MAX_PLAYERS)
	{
		ASSERT( false,"Player number too high in scrThreatInArea" );
		return(false);
	}

	totalThreat = 0;

	for(psDroid = apsDroidLists[playerTarget]; psDroid; psDroid = psDroid->psNext)
	{
		if (!objHasWeapon((BASE_OBJECT *)psDroid))
		{
			continue;
		}

		dX = psDroid->pos.x;
		dY = psDroid->pos.y;
		/* Do we care if the droid is visible or not */
		if(bVisible ? psDroid->visible[playerLooking] : true)
		{
			/* Have we found a droid in this area */
			if(dX >= x1 && dX <=x2 && dY >= y1 && dY <= y2)
			{
				switch ((asBodyStats + psDroid->asBits[COMP_BODY].nStat)->size)
				{
				case SIZE_LIGHT:
					totalThreat += ldThreat;
					break;
				case SIZE_MEDIUM:
					totalThreat += mdThreat;
					break;
				case SIZE_HEAVY:
				case SIZE_SUPER_HEAVY:
					totalThreat += hdThreat;
					break;
				default:
					ASSERT( false, "Weird droid size in threat assessment" );
					break;
				}
			}
		}
	}
//	DBPRINTF(("scrThreatInArea: returning %d\n", totalThreat));
	scrFunctionResult.v.ival = totalThreat;
	if(!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		ASSERT( false,"SCRIPT : Cannot push scrFunctionResult in scrThreatInArea" );
		return(false);
	}
	return(true);
}
// -----------------------------------------------------------------------------------------
// returns the nearest gateway bottleneck to a specified point
WZ_DECL_UNUSED static BOOL scrGetNearestGateway( void )
{
	SDWORD	x,y;
	UDWORD	nearestSoFar;
	UDWORD	dist;
	GATEWAY	*psGateway;
	SDWORD	retX,retY;
	SDWORD	*rX,*rY;
	BOOL	success;

	if(!stackPopParams(4, VAL_INT, &x, VAL_INT, &y, VAL_REF|VAL_INT, &rX, VAL_REF|VAL_INT, &rY))
	{
		ASSERT( false,"SCRIPT : Cannot get parameters for scrGetNearestGateway" );
		return(false);
	}

	if(x<0 || x>(SDWORD)mapWidth || y<0 || y>(SDWORD)mapHeight)
	{
		ASSERT( false,"SCRIPT : Invalid coordinates in getNearestGateway" );
		return(false);
	}

	nearestSoFar = UDWORD_MAX;
	retX = retY = -1;
	success = false;
	for (psGateway = gwGetGateways(); psGateway; psGateway = psGateway->psNext)
	{
		/* Get gateway midpoint */
		const int gX = (psGateway->x1 + psGateway->x2)/2;
		const int gY = (psGateway->y1 + psGateway->y2)/2;

		/* Estimate the distance to it */
		dist = hypotf(x - gX, y - gY);

		/* Is it best we've found? */
		if(dist<nearestSoFar)
		{
			success = true;
			/* Yes, then keep a record of it */
			nearestSoFar = dist;
			retX = gX;
			retY = gY;
		}
	}

	*rX = retX;
	*rY = retY;

	scrFunctionResult.v.bval = success;
	if(!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		ASSERT( false,"SCRIPT : Cannot return scrFunctionResult for stackPushResult" );
		return(false);
	}


	return(true);
}
// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL	scrSetWaterTile(void)
{
UDWORD	tileNum;

	if(!stackPopParams(1,VAL_INT, &tileNum))
	{
		ASSERT( false,"SCRIPT : Cannot get parameter for scrSetWaterTile" );
		return(false);
	}


	if(tileNum > 96)
	{
		ASSERT( false,"SCRIPT : Water tile number too high in scrSetWaterTile" );
		return(false);
	}

	setUnderwaterTile(tileNum);

	return(true);
}
// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL	scrSetRubbleTile(void)
{
UDWORD	tileNum;

	if(!stackPopParams(1,VAL_INT, &tileNum))
	{
		ASSERT( false,"SCRIPT : Cannot get parameter for scrSetRubbleTile" );
		return(false);
	}


	if(tileNum > 96)
	{
		ASSERT( false,"SCRIPT : Rubble tile number too high in scrSetWaterTile" );
		return(false);
	}

	setRubbleTile(tileNum);

	return(true);
}
// -----------------------------------------------------------------------------------------
static int scrSetCampaignNumber(lua_State *L)
{
	int campaignNumber = luaL_checkint(L, 1);

	setCampaignNumber(campaignNumber);
	return 0;
}

// -----------------------------------------------------------------------------------------
/// Tests whether a structure has a certain module for a player. Tests whether any structure
/// has this module if structure is null
static int scrTestStructureModule(lua_State *L)
{
	STRUCTURE *psStruct;
	BOOL bFound;
	int player = luaWZ_checkplayer(L, 1);

	/* Nothing yet */
	bFound = false;

	/* Check the specified case first */
	if(!lua_isnil(L, 2))
	{
		psStruct = (STRUCTURE*)luaWZObj_checkobject(L, 2, OBJ_STRUCTURE);
		if (psStruct == NULL)
		{
			luaL_error(L, "Testing for a module from a NULL struct - huh!?");
		}

		/* Fail if the structure isn't built yet */
		if (psStruct->status != SS_BUILT)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		if (structHasModule(psStruct))
		{
			bFound = true;
		}
	}
	/* psStructure was NULL - so test the general case */
	else
	{
		// Search them all, but exit if we get one!!
		for(psStruct = apsStructLists[player],bFound = false;
			psStruct && !bFound; psStruct = psStruct->psNext)
		{
			if (psStruct->status == SS_BUILT && structHasModule(psStruct))
			{
				bFound = true;
			}
		}
	}
	lua_pushboolean(L, bFound);
	return 1;
}


// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL	scrForceDamage( void )
{
DROID		*psDroid;
STRUCTURE	*psStructure;
FEATURE		*psFeature;
BASE_OBJECT	*psObj;
UDWORD		damagePercent;
float		divisor;
UDWORD		newVal;

	/* OK - let's get the vars */
	if(!stackPopParams(2,ST_BASEOBJECT,&psObj,VAL_INT,&damagePercent))
	{
		ASSERT( false,"Cannot pop params for scrForceDamage" );
		return(false);
	}

	/* Got to be a percent, so must be less than or equal to 100 */
	if(damagePercent > 100)
	{
		ASSERT( false,"scrForceDamage : You're supposed to be passing in a PERCENTAGE VALUE, \
			instead I got given %d, which is clearly no good, now is it!?", damagePercent );
		return(false);
	}

	/* Get percentage in range [0.1] */
	divisor =  (float)damagePercent / 100.f;

	/* See what we're dealing with */
	switch(psObj->type)
	{
	case OBJ_DROID:
		psDroid = (DROID *) psObj;
		newVal = divisor * psDroid->originalBody;
		psDroid->body = newVal;
		break;
	case OBJ_STRUCTURE:
		psStructure = (STRUCTURE *) psObj;
		newVal = divisor * structureBody(psStructure);
		psStructure->body = (UWORD)newVal;
		break;
	case OBJ_FEATURE:
		psFeature = (FEATURE *) psObj;
		/* Some features cannot be damaged */
		if(psFeature->psStats->damageable)
		{
			newVal = divisor * psFeature->psStats->body;
			psFeature->body = newVal;
		}
		break;
	default:
		ASSERT( false,"Unsupported base object type in scrForceDamage" );
		return(false);
		break;
	}

	return(true);

}
// Kills of a droid without spawning any explosion effects.
// -----------------------------------------------------------------------------------------
WZ_DECL_UNUSED static BOOL	scrDestroyUnitsInArea( void )
{
DROID	*psDroid,*psNext;
SDWORD	x1,y1,x2,y2;
UDWORD	player;
UDWORD	count=0;

	if(!stackPopParams(5,VAL_INT,&x1,VAL_INT,&y1,VAL_INT,&x2,VAL_INT,&y2,VAL_INT, &player))
	{
		ASSERT( false,"Cannot get params for scrDestroyUnitsInArea" );
		return(false);
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false,"Invalid player number in scrKillDroidsInArea" );
	}

	for(psDroid = apsDroidLists[player]; psDroid; psDroid = psNext)
	{
		psNext = psDroid->psNext;	// get a copy cos pointer will be lost
		if( (psDroid->pos.x > x1) && (psDroid->pos.x < x2) &&
			(psDroid->pos.y > y1) && (psDroid->pos.y < y2) )
		{
			/* then it's inside the area */
			SendDestroyDroid(psDroid);
			count++;
		}
	}

	scrFunctionResult.v.ival = count;
	if(!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return(false);
	}

	return(true);
}

// -----------------------------------------------------------------------------------------
static int scrRemoveDroid(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	destroyDroid(psDroid);
	return 0;
}

// -----------------------------------------------------------------------------------------
static bool structHasModule(STRUCTURE *psStruct)
{
	STRUCTURE_STATS	*psStats;
	BOOL		bFound;

	/* Not found yet */
	bFound = false;

	if(psStruct)
	{
		/* Grab a stats pointer */
		psStats = psStruct->pStructureType;
		if(StructIsFactory(psStruct)
			|| psStats->type == REF_POWER_GEN || psStats->type == REF_RESEARCH)
		{
			switch(psStats->type)
			{
				case REF_POWER_GEN:
					if (((POWER_GEN *)psStruct->pFunctionality)->capacity)
					{
						bFound = true;
					}
					break;
				case REF_FACTORY:
				case REF_VTOL_FACTORY:
					if (((FACTORY *)psStruct->pFunctionality)->capacity)
					{
						bFound = true;
					}
					break;
				case REF_RESEARCH:
					if (((RESEARCH_FACILITY *)psStruct->pFunctionality)->capacity)

					{
						bFound = true;
					}
					break;
				default:
					//no other structures can have modules attached
					break;
			}
		}
		else
		{
			/* Wrong type of building - cannot have a module */
			bFound = false;
		}

	}
	return true;
}

// -----------------------------------------------------------------------------------------
// give player a template belonging to another.
WZ_DECL_UNUSED static BOOL scrAddTemplate(void)
{
	DROID_TEMPLATE *psTemplate;
	UDWORD			player;

	if (!stackPopParams(2, ST_TEMPLATE, &psTemplate, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddTemplate:player number is too high" );
		return false;
	}

	ASSERT( psTemplate != NULL, "scrAddTemplate: Invalid template pointer" );

	if(	addTemplate(player,psTemplate))
	{
		scrFunctionResult.v.bval = true;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			return false;
		}
	}
	else
	{
		scrFunctionResult.v.bval = false;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			return false;
		}
	}

	return true;
}



// -----------------------------------------------------------------------------------------

// additional structure check
static BOOL structDoubleCheck(BASE_STATS *psStat,UDWORD xx,UDWORD yy, SDWORD maxBlockingTiles)
{
	UDWORD		x, y, xTL, yTL, xBR, yBR;
	UBYTE		count = 0;
	STRUCTURE_STATS	*psBuilding = (STRUCTURE_STATS *)psStat;
	GATEWAY		*psGate;

	xTL = xx-1;
	yTL = yy-1;
	xBR = (xx + psBuilding->baseWidth );
	yBR = (yy + psBuilding->baseBreadth );

	// check against building in a gateway, as this can seriously block AI passages
	for (psGate = gwGetGateways(); psGate; psGate = psGate->psNext)
	{
		for (x = xx; x <= xBR; x++)
		{
			for (y =yy; y <= yBR; y++)
			{
				if ((x >= psGate->x1 && x <= psGate->x2) && (y >= psGate->y1 && y <= psGate->y2))
				{
					return false;
				}
			}
		}
	}

	// can you get past it?
	y = yTL;	// top
	for(x = xTL;x!=xBR+1;x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	y = yBR;	// bottom
	for(x = xTL;x!=xBR+1;x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xTL;	// left
	for(y = yTL+1; y!=yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xBR;	// right
	for(y = yTL+1; y!=yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	//make sure this location is not blocked from too many sides
	if((count <= maxBlockingTiles) || (maxBlockingTiles == -1))
	{
		return true;
	}
	return false;
}

static BOOL pickStructLocation(DROID *psDroid, int index, int *pX, int *pY, int player, int maxBlockingTiles)
{
	STRUCTURE_STATS	*psStat;
	UDWORD			numIterations = 30;
	BOOL			found = false;
	int startX, startY, incX, incY, x, y;

	ASSERT_OR_RETURN(false, player < MAX_PLAYERS && player >= 0, "Invalid player number %d", player);

	// check for wacky coords.
	if (*pX < 0 || *pX > world_coord(mapWidth) || *pY < 0 || *pY > world_coord(mapHeight))
	{
		goto endstructloc;
	}

	psStat = &asStructureStats[index];			// get stat.
	startX = map_coord(*pX);				// change to tile coords.
	startY = map_coord(*pY);
	x = startX;
	y = startY;

	// save a lot of typing... checks whether a position is valid
	#define LOC_OK(_x, _y) (_x >= 0 && _y >= 0 && _x < mapWidth && _y < mapHeight && \
	                        (!psDroid || fpathCheck(psDroid->pos, Vector3i_Init(world_coord(_x), world_coord(_y), 0), PROPULSION_TYPE_WHEELED)) \
	                        && validLocation((BASE_STATS*)psStat, _x, _y, 0, player, false) && structDoubleCheck((BASE_STATS*)psStat, _x, _y, maxBlockingTiles))

	// first try the original location
	if (LOC_OK(startX, startY))
	{
		found = true;
	}

	// try some locations nearby
	for (incX = 1, incY = 1; incX < numIterations && !found; incX++, incY++)
	{
		y = startY - incY;	// top
		for (x = startX - incX; x < (SDWORD)(startX + incX); x++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX + incX;	// right
		for (y = startY - incY; y < (SDWORD)(startY + incY); y++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		y = startY + incY;	// bottom
		for (x = startX + incX; x > (SDWORD)(startX - incX); x--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX - incX;	// left
		for (y = startY + incY; y > (SDWORD)(startY - incY); y--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
	}

endstructloc:
	// back to world coords.
	if (found)	// did it!
	{
		*pX = world_coord(x) + (psStat->baseWidth * (TILE_UNITS / 2));
		*pY = world_coord(y) + (psStat->baseBreadth * (TILE_UNITS / 2));
	}
	return found;
}

/// pick a structure location(only used in skirmish game at 27Aug) ajl.
static int scrPickStructLocation(lua_State *L)
{
	int index = luaWZObj_checkstructurestat(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int player = luaWZ_checkplayer(L, 4);
	
	BOOL success = pickStructLocation(NULL, index, &x, &y, player, MAX_BLOCKING_TILES);
	
	lua_pushboolean(L, success);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 3;
}

// pick a structure location and check that we can build there (duh!)
static int scrPickStructLocationC(lua_State *L)
{
	DROID *psDroid = (DROID *)luaWZObj_checkobject(L, 1, OBJ_DROID);
	int index = luaWZObj_checkstructurestat(L, 2);
	int x = luaL_checkinteger(L, 3);
	int y = luaL_checkinteger(L, 4);
	int player = luaWZ_checkplayer(L, 5);
	int maxBlockingTiles = luaL_checkinteger(L, 6);

	BOOL success = pickStructLocation(psDroid, index, &x, &y, player, maxBlockingTiles);
	
	lua_pushboolean(L, success);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 3;
}

/// pick a structure location(only used in skirmish game at 27Aug) ajl.
/// Max number of blocking tiles is passed as parameter for this one
static int scrPickStructLocationB(lua_State *L) // FIXME, duplicated code
{
	int index = luaWZObj_checkstructurestat(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int player = luaWZ_checkplayer(L, 4);
	int maxBlockingTiles = luaL_checkinteger(L, 5);

	BOOL success = pickStructLocation(NULL, index, &x, &y, player, maxBlockingTiles);
	
	lua_pushboolean(L, success);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 3;
}

// -----------------------------------------------------------------------------------------
// Sets the transporter entry and exit points for the map
static int scrSetTransporterExit(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int exitTileX = luaL_checkinteger(L, 2);
	int exitTileY = luaL_checkinteger(L, 3);

	missionSetTransporterExit(player, exitTileX, exitTileY);

	return 0;
}

// -----------------------------------------------------------------------------------------
// Fly transporters in at start of map
static int scrFlyTransporterIn(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int entryTileX = luaL_checkinteger(L, 2);
	int entryTileY = luaL_checkinteger(L, 3);
	bool trackTransporter = luaL_checkboolean(L, 4);

	missionSetTransporterEntry(player, entryTileX, entryTileY);
	missionFlyTransportersIn(player, trackTransporter);

	return 0;
}

// -----------------------------------------------------------------------------------------



/*
 ** scrGetGameStatus
 *
 *  PARAMETERS: The parameter passed must be one of the STATUS_ variable
 *
 *  DESCRIPTION: Returns various BOOL options in the game	e.g. If the reticule is open
 *	  - You should use the externed variable intMode for other game mode options
 *		e.g. in the intelligence screen or desgin screen)
 *
 *  RETURNS:
 *
 */
WZ_DECL_UNUSED static BOOL scrGetGameStatus(void)
{
	SDWORD GameChoice;
	BOOL bResult;

	if (!stackPopParams(1, VAL_INT, &GameChoice))
	{
		return false;
	}

//	DBPRINTF(("getgamestatus choice=%d\n",GameChoice));

	bResult=false;		// the default scrFunctionResult is false

	switch (GameChoice)
	{

		case STATUS_ReticuleIsOpen:
			if(widgGetFromID(psWScreen,IDRET_FORM) != NULL) bResult=true;
			break;

		case STATUS_BattleMapViewEnabled:
//			if (driveTacticalActive()==true) scrFunctionResult=true;


			if (bResult==true)
			{
				debug( LOG_NEVER, "battle map active" );
			}
			else
			{
				debug( LOG_NEVER, "battle map not active" );
			}


			break;
		case STATUS_DeliveryReposInProgress:
			if (DeliveryReposValid()==true) bResult=true;
			break;

		default:
		ASSERT( false,"ScrGetGameStatus. Invalid STATUS_ variable" );
		break;
	}

	scrFunctionResult.v.bval = bResult;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

//get the colour number used by a player
WZ_DECL_UNUSED static BOOL scrGetPlayerColour(void)
{
	SDWORD		player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGetPlayerColour: player number is too high" );
		return false;
	}

	scrFunctionResult.v.ival = (SDWORD)getPlayerColour(player);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/// get the colour name of the player ("green", "black" etc)
static int scrGetPlayerColourName(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);

	lua_pushstring(L, getPlayerColourName(player));
	return 1;
}

//set the colour number to use for a player
WZ_DECL_UNUSED static BOOL scrSetPlayerColour(void)
{
	SDWORD		player, colour;

	if (!stackPopParams(2, VAL_INT, &colour, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetPlayerColour:player number is too high" );
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetPlayerColour:colour number is too high" );
		return false;
	}

	//not the end of the world if this doesn't work so don't check the return code
	(void)setPlayerColour(player, colour);

	return true;
}

/// set all droids in an area to belong to a different player - returns the number of droids changed
static int scrTakeOverDroidsInArea(lua_State *L)
{
	int numChanged;
	DROID *psDroid, *psNext;
	
	int fromPlayer = luaWZ_checkplayer(L, 1);
	int toPlayer = luaWZ_checkplayer(L, 2);
	int x1 = luaL_checkint(L, 3);
	int y1 = luaL_checkint(L, 4);
	int x2 = luaL_checkint(L, 5);
	int y2 = luaL_checkint(L, 6);

	if (x1 > world_coord(MAP_MAXWIDTH))
	{
		return luaL_error(L, "scrTakeOverUnitsInArea: x1 is greater than max mapWidth" );
	}

	if (x2 > world_coord(MAP_MAXWIDTH))
	{
		return luaL_error(L, "scrTakeOverUnitsInArea: x2 is greater than max mapWidth" );
	}

	if (y1 > world_coord(MAP_MAXHEIGHT))
	{
		return luaL_error(L, "scrTakeOverUnitsInArea: y1 is greater than max mapHeight" );
	}

	if (y2 > world_coord(MAP_MAXHEIGHT))
	{
		return luaL_error(L, "scrTakeOverUnitsInArea: y2 is greater than max mapHeight" );
	}

	numChanged = 0;
	for (psDroid = apsDroidLists[fromPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		//check if within area specified
		if (psDroid->pos.x >= x1 && psDroid->pos.x <= x2 &&
			psDroid->pos.y >= y1 && psDroid->pos.y <= y2)
		{
			//give the droid away
			if (giftSingleDroid(psDroid, toPlayer))
			{
				numChanged++;
			}
		}
	}

	lua_pushinteger(L, numChanged);
	return 1;
}

/*this takes over a single droid and passes a pointer back to the new one*/
WZ_DECL_UNUSED static BOOL scrTakeOverSingleDroid(void)
{
	SDWORD			playerToGain;
	DROID		   *psDroidToTake, *psNewDroid;

    if (!stackPopParams(2, ST_DROID, &psDroidToTake, VAL_INT, &playerToGain))
    {
		return false;
    }

	if (psDroidToTake == NULL)
	{
		ASSERT( false, "scrTakeOverSingleUnit:player number is too high" );
		return false;
	}

    if (psDroidToTake == NULL)
    {
        ASSERT( false, "scrTakeOverSingleUnit: Null unit" );
        return false;
    }

	ASSERT( psDroidToTake != NULL,
		"scrTakeOverSingleUnit: Invalid unit pointer" );

	psNewDroid = giftSingleDroid(psDroidToTake, playerToGain);

	scrFunctionResult.v.oval = psNewDroid;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
    }
	return true;
}

// set all droids in an area of a certain experience level or less to belong to
// a different player - returns the number of droids changed
WZ_DECL_UNUSED static BOOL scrTakeOverDroidsInAreaExp(void)
{
	SDWORD		fromPlayer, toPlayer, x1, x2, y1, y2, numChanged, level, maxUnits;
	DROID	   *psDroid, *psNext;

	if (!stackPopParams(8, VAL_INT, &fromPlayer, VAL_INT, &toPlayer,
		VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2, VAL_INT, &level, VAL_INT, &maxUnits))
	{
		return false;
	}

	if (fromPlayer >= MAX_PLAYERS || toPlayer >= MAX_PLAYERS)
	{
		ASSERT( false, "scrTakeOverUnitsInArea:player number is too high" );
		return false;
	}

	if (x1 > world_coord(MAP_MAXWIDTH))
	{
		ASSERT( false, "scrTakeOverUnitsInArea: x1 is greater than max mapWidth" );
		return false;
	}

	if (x2 > world_coord(MAP_MAXWIDTH))
	{
		ASSERT( false, "scrTakeOverUnitsInArea: x2 is greater than max mapWidth" );
		return false;
	}

	if (y1 > world_coord(MAP_MAXHEIGHT))
	{
		ASSERT( false, "scrTakeOverUnitsInArea: y1 is greater than max mapHeight" );
		return false;
	}

	if (y2 > world_coord(MAP_MAXHEIGHT))
	{
		ASSERT( false, "scrTakeOverUnitsInArea: y2 is greater than max mapHeight" );
		return false;
	}

	numChanged = 0;
	for (psDroid = apsDroidLists[fromPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		//check if within area specified
		if ((psDroid->droidType != DROID_CONSTRUCT) &&
			(psDroid->droidType != DROID_REPAIR) &&
			(psDroid->droidType != DROID_CYBORG_CONSTRUCT) &&
			(psDroid->droidType != DROID_CYBORG_REPAIR) &&
//			((SDWORD)getDroidLevel(psDroid) <= level) &&
			((SDWORD)psDroid->experience/65536 <= level) &&
			psDroid->pos.x >= x1 && psDroid->pos.x <= x2 &&
			psDroid->pos.y >= y1 && psDroid->pos.y <= y2)
		{
			//give the droid away
			if (giftSingleDroid(psDroid, toPlayer))
			{
				numChanged++;
			}
		}

		if (numChanged >= maxUnits)
		{
			break;
		}
	}

	scrFunctionResult.v.ival = numChanged;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

    return true;
}

/*this takes over a single structure and passes a pointer back to the new one*/
WZ_DECL_UNUSED static BOOL scrTakeOverSingleStructure(void)
{
	SDWORD			playerToGain;
	STRUCTURE	   *psStructToTake, *psNewStruct;
	UDWORD		  structureInc;

    if (!stackPopParams(2, ST_STRUCTURE, &psStructToTake, VAL_INT, &playerToGain))
    {
		return false;
    }

	if (psStructToTake == NULL)
	{
		ASSERT( false, "scrTakeOverSingleStructure:player number is too high" );
		return false;
	}

    if (psStructToTake == NULL)
    {
        ASSERT( false, "scrTakeOverSingleStructure: Null structure" );
        return false;
    }

	ASSERT( psStructToTake != NULL,
		"scrTakeOverSingleStructure: Invalid structure pointer" );

	structureInc = psStructToTake->pStructureType->ref - REF_STRUCTURE_START;
	if (playerToGain == (SDWORD)selectedPlayer && StructIsFactory(psStructToTake) &&
		asStructLimits[playerToGain][structureInc].currentQuantity >= MAX_FACTORY)
	{
		debug( LOG_NEVER, "scrTakeOverSingleStructure - factory ignored for selectedPlayer\n" );
        psNewStruct = NULL;
    }
    else
    {
        psNewStruct = giftSingleStructure(psStructToTake, (UBYTE)playerToGain, true);
        if (psNewStruct)
        {
            //check the structure limits aren't compromised
            if (asStructLimits[playerToGain][structureInc].currentQuantity >
                asStructLimits[playerToGain][structureInc].limit)
            {
                asStructLimits[playerToGain][structureInc].limit = asStructLimits[
                    playerToGain][structureInc].currentQuantity;
            }
            //for each structure taken - add graphical effect if the selectedPlayer
            if (playerToGain == (SDWORD)selectedPlayer)
            {
                assignSensorTarget((BASE_OBJECT *)psNewStruct);
            }
        }
    }

	scrFunctionResult.v.oval = psNewStruct;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
    }
	return true;
}

//set all structures in an area to belong to a different player - returns the number of droids changed
//will not work on factories for the selectedPlayer
WZ_DECL_UNUSED static BOOL scrTakeOverStructsInArea(void)
{
	SDWORD		fromPlayer, toPlayer, x1, x2, y1, y2, numChanged;
    STRUCTURE   *psStruct, *psNext, *psNewStruct;
    UDWORD      structureInc;

	if (!stackPopParams(6, VAL_INT, &fromPlayer, VAL_INT, &toPlayer,
        VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (fromPlayer >= MAX_PLAYERS || toPlayer >= MAX_PLAYERS)
	{
		ASSERT( false, "scrTakeOverStructsInArea:player number is too high" );
		return false;
	}

	if (x1 > world_coord(MAP_MAXWIDTH))
	{
		ASSERT( false, "scrTakeOverStructsInArea: x1 is greater than max mapWidth" );
		return false;
	}

    if (x2 > world_coord(MAP_MAXWIDTH))
	{
		ASSERT( false, "scrTakeOverStructsInArea: x2 is greater than max mapWidth" );
		return false;
	}

    if (y1 > world_coord(MAP_MAXHEIGHT))
	{
		ASSERT( false, "scrTakeOverStructsInArea: y1 is greater than max mapHeight" );
		return false;
	}

    if (y2 > world_coord(MAP_MAXHEIGHT))
	{
		ASSERT( false, "scrTakeOverStructsInArea: y2 is greater than max mapHeight" );
		return false;
	}

    numChanged = 0;
    for (psStruct = apsStructLists[fromPlayer]; psStruct != NULL; psStruct = psNext)
    {
        psNext = psStruct->psNext;
        //check if within area specified
        if (psStruct->pos.x >= x1 && psStruct->pos.x <= x2 &&
            psStruct->pos.y >= y1 && psStruct->pos.y <= y2)
        {
            //changed this so allows takeOver is have less than 5 factories
            //don't work on factories for the selectedPlayer
            structureInc = psStruct->pStructureType->ref - REF_STRUCTURE_START;
            if (toPlayer == (SDWORD)selectedPlayer && StructIsFactory(psStruct) &&
                asStructLimits[toPlayer][structureInc].currentQuantity >= MAX_FACTORY)
            {
				debug( LOG_NEVER, "scrTakeOverStructsInArea - factory ignored for selectedPlayer\n" );
            }
            else
            {
                //give the structure away
                psNewStruct = giftSingleStructure(psStruct, (UBYTE)toPlayer, true);
                if (psNewStruct)
                {
                    numChanged++;
                    //check the structure limits aren't compromised
                    //structureInc = psNewStruct->pStructureType->ref - REF_STRUCTURE_START;
                    if (asStructLimits[toPlayer][structureInc].currentQuantity >
                        asStructLimits[toPlayer][structureInc].limit)
                    {
                        asStructLimits[toPlayer][structureInc].limit = asStructLimits[
                            toPlayer][structureInc].currentQuantity;
                    }
                    //for each structure taken - add graphical effect if the selectedPlayer
                    if (toPlayer == (SDWORD)selectedPlayer)
                    {
                        assignSensorTarget((BASE_OBJECT *)psNewStruct);
                    }
                }
            }
        }
    }

	scrFunctionResult.v.ival = numChanged;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

    return true;
}

//set Flag for defining what happens to the droids in a Transporter
WZ_DECL_UNUSED static BOOL scrSetDroidsToSafetyFlag(void)
{
	BOOL bState;

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}

	setDroidsToSafetyFlag(bState);

	return true;
}

//set Flag for defining whether the coded countDown is called
WZ_DECL_UNUSED static BOOL scrSetPlayCountDown(void)
{
	BOOL bState;

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}


	setPlayCountDown((UBYTE)bState);


	return true;
}

/// get the number of droids currently onthe map for a player
static int scrGetDroidCount(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);

	lua_pushinteger(L, getNumDroids(player));
	return 1;
}


// fire a weapon stat at an object
WZ_DECL_UNUSED static BOOL scrFireWeaponAtObj(void)
{
	Vector3i target;
	BASE_OBJECT *psTarget;
	WEAPON sWeapon;

	memset(&sWeapon, 0, sizeof(sWeapon));
	if (!stackPopParams(2, ST_WEAPON, &sWeapon.nStat, ST_BASEOBJECT, &psTarget))
	{
		return false;
	}

	if (psTarget == NULL)
	{
		ASSERT( false,"scrFireWeaponAtObj: Null target pointer" );
		return false;
	}

	target = psTarget->pos;

	// send the projectile using the selectedPlayer so that it can always be seen
	proj_SendProjectile(&sWeapon, NULL, selectedPlayer, target, psTarget, true, 0);

	return true;
}

// fire a weapon stat at a location
WZ_DECL_UNUSED static BOOL scrFireWeaponAtLoc(void)
{
	Vector3i target;
	WEAPON sWeapon;

	memset(&sWeapon, 0, sizeof(sWeapon));
	if (!stackPopParams(3, ST_WEAPON, &sWeapon.nStat, VAL_INT, &target.x, VAL_INT, &target.y))
	{
		return false;
	}

	target.z = map_Height(target.x, target.y);

	// send the projectile using the selectedPlayer so that it can always be seen
	proj_SendProjectile(&sWeapon, NULL, selectedPlayer, target, NULL, true, 0);

	return true;
}

// set the number of kills for a droid
WZ_DECL_UNUSED static BOOL scrSetDroidKills(void)
{
	DROID	*psDroid;
	SDWORD	kills;

	if (!stackPopParams(2, ST_DROID, &psDroid, VAL_INT, &kills))
	{
		return true;
	}

	if ((psDroid == NULL) ||
		(psDroid->type != OBJ_DROID))
	{
		ASSERT( false, "scrSetUnitKills: NULL/invalid unit pointer" );
		return false;
	}

	psDroid->experience = kills * 100 * 65536;

	return true;
}

// get the number of kills for a droid
WZ_DECL_UNUSED static BOOL scrGetDroidKills(void)
{
	DROID	*psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		return true;
	}

	if ((psDroid == NULL) ||
		(psDroid->type != OBJ_DROID))
	{
		ASSERT( false, "scrGetDroidKills: NULL/invalid unit pointer" );
		return false;
	}

	scrFunctionResult.v.ival = psDroid->experience/65536;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// reset the visibility for a player
WZ_DECL_UNUSED static BOOL scrResetPlayerVisibility(void)
{
	SDWORD			player, i;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player < 0 || player > MAX_PLAYERS)
	{
		ASSERT( false, "scrResetPlayerVisibility: invalid player" );
		return false;
	}

	for(i=0; i< MAX_PLAYERS; i++)
	{
		if (i == player)
		{
			continue;
		}

		for(psObj = (BASE_OBJECT *)apsDroidLists[i]; psObj; psObj = psObj->psNext)
		{
			psObj->visible[player] = 0;
		}

		for(psObj = (BASE_OBJECT *)apsStructLists[i]; psObj; psObj = psObj->psNext)
		{
			psObj->visible[player] = 0;
		}
	}

	for(psObj = (BASE_OBJECT *)apsFeatureLists[0]; psObj; psObj = psObj->psNext)
	{
		psObj->visible[player] = 0;
	}

	clustResetVisibility(player);

	return true;
}


// set the vtol return pos for a player
WZ_DECL_UNUSED static BOOL scrSetVTOLReturnPos(void)
{
	SDWORD		player, tx,ty;

	if (!stackPopParams(3, VAL_INT, &player, VAL_INT, &tx, VAL_INT, &ty))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetVTOLReturnPos: invalid player" );
			return false;
	}

	asVTOLReturnPos[player].x = (tx * TILE_UNITS) + TILE_UNITS/2;
	asVTOLReturnPos[player].y = (ty * TILE_UNITS) + TILE_UNITS/2;

	return true;
}

//called via the script in a Limbo Expand level to set the level to plain ol' expand
WZ_DECL_UNUSED static BOOL scrResetLimboMission(void)
{
    //check currently on a Limbo expand mission
    if (!missionLimboExpand())
    {
        ASSERT( false, "scrResetLimboMission: current mission type invalid" );
        return false;
    }

	//turn it into an expand mission
	resetLimboMission();

    return true;
}

/// is this a VTOL droid?
static int scrIsVtol(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);

	lua_pushboolean(L, isVtolDroid(psDroid));
	return 1;
}




// do the setting up of the template list for the tutorial.
WZ_DECL_UNUSED static BOOL scrTutorialTemplates(void)
{
	DROID_TEMPLATE	*psCurr, *psPrev;
	char			pName[MAX_STR_LENGTH];

	// find ViperLtMGWheels
	sstrcpy(pName, "ViperLtMGWheels");

	getDroidResourceName(pName);


	for (psCurr = apsDroidTemplates[selectedPlayer],psPrev = NULL;
			psCurr != NULL;
			psCurr = psCurr->psNext)
	{
		if (strcmp(pName,psCurr->aName)==0)
		{
			if (psPrev)
			{
				psPrev->psNext = psCurr->psNext;
			}
			else
			{
				apsDroidTemplates[selectedPlayer] = psCurr->psNext;
			}
			//quit looking cos found
			break;
		}
		psPrev = psCurr;
	}

	// Delete the template.
	if(psCurr)
	{
		free(psCurr);
	}
	else
	{
		debug( LOG_FATAL, "tutorial template setup failed" );
		abort();
		return false;
	}
	return true;
}







//-----------------------------------------
//New functions
//-----------------------------------------

//compare two strings (0 means they are different)
WZ_DECL_UNUSED static BOOL scrStrcmp(void)
{
	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_STRING, &strParam2))
	{
		debug(LOG_ERROR, "scrStrcmp(): stack failed");
		return false;
	}

	scrFunctionResult.v.bval = !strcmp(strParam1, strParam2);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrStrcmp: failed to push result");
		return false;
	}

	return true;
}

/* Output a string to console */
static int scrConsole(lua_State *L)
{
	addConsoleMessage(luaL_checkstring(L, 1),DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	return 0;
}

WZ_DECL_UNUSED static BOOL scrDebug[MAX_PLAYERS];

/// turn on debug messages
static int scrDbgMsgOn(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	BOOL bOn = luaL_checkboolean(L, 2);

	scrDebug[player] = bOn;

	return 0;
}

static int scrMsg(lua_State *L)
{
	char tmp[255];
	const char *message = luaL_checkstring(L, 1);
	int playerFrom      = luaWZ_checkplayer(L, 2);
	int playerTo        = luaWZ_checkplayer(L, 3);

	sendAIMessage((char*)message, playerFrom, playerTo);

	//show the message we sent on our local console as well (even in skirmish, if player plays as this AI)
	if(playerFrom == selectedPlayer)
	{
		snprintf(tmp, sizeof(tmp), "[%d-%d] : %s",playerFrom, playerTo, message);											// add message
		addConsoleMessage(tmp, RIGHT_JUSTIFY, playerFrom);
	}
	return 0;
}

/// Print a debug message to the console
static int scrDbg(lua_State *L)
{
	const char *message = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	if(scrDebug[player])
	{
		char	sTmp[255];
		snprintf(sTmp, 255, "%d) %s", player, message);
		addConsoleMessage(sTmp,DEFAULT_JUSTIFY, player);
		debug(LOG_SCRIPT, "dbg: %s", sTmp);
	}

	return 0;
}

static int scrDebugFile(lua_State *L)
{
	const char *message = luaL_checkstring(L, 1);
	
	debug(LOG_SCRIPT, "%s", message);

	return 0;
}

static int scrInternalGetDroidVisibleBy(lua_State *L)
{
	int player         = luaWZ_checkplayer(L, 1);
	int seenBy         = luaWZ_checkplayer(L, 2);
	int enumDroidCount = luaL_checkint(L, 3);
	int count;
	DROID *psDroid;

	count = 0;
	for(psDroid=apsDroidLists[player];psDroid && count<enumDroidCount;count++)
	{
		psDroid = psDroid->psNext;
	}

	while(psDroid)
	{
		enumDroidCount++;
		if(psDroid->visible[seenBy])
		{
			luaWZObj_pushdroid(L, psDroid);
			lua_pushinteger(L, enumDroidCount);
			return 2;
		}
		psDroid = psDroid->psNext;
	}
	return 0;
}

static int scrInternalGetStructureVisibleBy(lua_State *L)
{
	int index;
	int player          = luaWZ_checkplayer(L, 2);
	int seenBy          = luaWZ_checkplayer(L, 3);
	int enumStructCount = luaL_checkint(L, 4);
	int count;
	STRUCTURE *psStruct;
	STRUCTURE_STATS *stat;

	// check if a structure stat was provided
	if (lua_isnil(L, 1))
	{
		// no, so we are looking for all structures
		stat = NULL;
	}
	else
	{
		index = luaWZObj_checkstructurestat(L, 1);
		stat = (STRUCTURE_STATS *)(asStructureStats + index);
	}

	// go to the correct start point in the structure list.
	count = 0;
	for (psStruct=apsStructLists[player];psStruct && count<enumStructCount;count++)
	{
		psStruct = psStruct->psNext;
	}

	if (psStruct == NULL)		// no more to find.
	{
		return 0;
	}

	while(psStruct)	// find a visible structure of required type.
	{
		enumStructCount++;
		if ((!stat || psStruct->pStructureType->ref == stat->ref) &&
			(psStruct->visible[seenBy]))
		{
			luaWZObj_pushstructure(L, psStruct);
			lua_pushinteger(L, enumStructCount);
			return 2;
		}
		psStruct = psStruct->psNext;
	}
	// no building found
	return 0;
}

static int scrInternalGetFeatureVisibleBy(lua_State *L)
{
	int seenBy           = luaL_checkint(L, 1);	// cannot be checkplayer since can be -1 for see all
	int featureType      = luaWZObj_checkfeaturestat(L, 2);
	int enumFeatureCount = luaL_checkint(L, 3);
	int count;
	FEATURE *psFeature;

	// go to the correct start point in the feature list.
	count = 0;
	for (psFeature=apsFeatureLists[0];psFeature && count<enumFeatureCount;count++)
	{
		psFeature = psFeature->psNext;
	}

	if (psFeature == NULL)		// no more to find.
	{
		return 0;
	}

	while(psFeature)	// find a visible feature of required type.
	{
		enumFeatureCount++;
		if ((featureType < 0 || psFeature->psStats->subType == featureType) &&
			(seenBy < 0 || psFeature->visible[seenBy]))
		{
			luaWZObj_pushfeature(L, psFeature);
			lua_pushinteger(L, enumFeatureCount);
			return 2;
		}
		psFeature = psFeature->psNext;
	}
	return 0;
}

/// Return the template factory is currently building
static int scrFactoryGetTemplate(lua_State *L)
{
	STRUCTURE *psStructure = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);
	DROID_TEMPLATE	*psTemplate = NULL;

	if(!StructIsFactory(psStructure))
	{
		return luaL_argerror(L, 1, "structure not a factory");
	}

	psTemplate = (DROID_TEMPLATE *)((FACTORY*)psStructure->pFunctionality)->psSubject;

	ASSERT( psTemplate != NULL,
		"scrFactoryGetTemplate: Invalid template pointer" );

	luaWZObj_pushtemplate(L, psTemplate);
	return 1;
}

static int scrNumTemplatesInProduction(lua_State *L)
{
	SDWORD			numTemplates = 0;
	STRUCTURE		*psStruct;
	STRUCTURE		*psList;
	BASE_STATS		*psBaseStats;
	
	DROID_TEMPLATE	*psTemplate = luaWZObj_checktemplate(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	psBaseStats = (BASE_STATS *)psTemplate; //Convert

	psList = apsStructLists[player];

	for (psStruct = psList; psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (StructIsFactory(psStruct))
		{
			FACTORY *psFactory = (FACTORY *)psStruct->pFunctionality;

			//if this is the template currently being worked on
			if (psBaseStats == psFactory->psSubject)
			{
				numTemplates++;
			}
		}
	}

	lua_pushinteger(L, numTemplates);
	return 1;
}

/// Returns number of units based on a component a certain player has
static int scrNumDroidsByComponent(lua_State *L)
{
	UDWORD				numFound;
	DROID				*psDroid;
	int comp, type;
	
	const char *name = luaWZObj_checkname(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	int lookingPlayer = luaWZ_checkplayer(L, 3);
	
	findComponentByName(name, &type, &comp);
	if (type < 0)
	{
		return luaL_error(L, "unknown component: \"%s\"", name);
	}
	
	numFound = 0;

	//check droids
	for(psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->visible[lookingPlayer])		//can see this droid?
		{
			if ( (type == COMP_WEAPON && psDroid->asWeaps[0].nStat == comp) ||
			     (type != COMP_WEAPON && psDroid->asBits[type].nStat == comp))
			{
				numFound++;
			}
		}
	}

	lua_pushinteger(L, numFound);
	return 1;
}

/// get the structure limit for the structure stat
static int scrGetStructureLimit(lua_State *L)
{
	STRUCTURE_LIMITS	*psStructLimits;
	
	int structInc = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	psStructLimits = asStructLimits[player];
	lua_pushinteger(L, psStructLimits[structInc].limit);
	return 1;
}

/// Returns true if limit for the passed structurestat is reached, otherwise returns false
static int scrStructureLimitReached(lua_State *L)
{
	STRUCTURE_LIMITS *psStructLimits;
	
	int structInc = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	psStructLimits = asStructLimits[player];

	lua_pushboolean(L, psStructLimits[structInc].currentQuantity
	                   >= psStructLimits[structInc].limit);
	return 1;
}

/// How many structures of a given type a player has
static int scrGetNumStructures(lua_State *L)
{
	int numStructures;
	STRUCTURE_LIMITS *psStructLimits;
	
	int structInc = luaWZObj_checkstructurestat(L, 1);
	int player = luaWZ_checkplayer(L, 2);

	psStructLimits = asStructLimits[player];
	numStructures = (SDWORD)psStructLimits[structInc].currentQuantity;

	lua_pushinteger(L, numStructures);
	return 1;
}

// Return player's unit limit
WZ_DECL_UNUSED static BOOL scrGetUnitLimit(void)
{
	SDWORD				player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrGetUnitLimit: failed to pop");
		return false;}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetStructureLimits:player number is too high" );
		return false;}

	scrFunctionResult.v.ival = getMaxDroids(player);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Return minimum of 2 vals
WZ_DECL_UNUSED static BOOL scrMin(void)
{
	SDWORD				val1,val2;

	if (!stackPopParams(2, VAL_INT, &val1, VAL_INT, &val2))
	{
		return false;
	}

	scrFunctionResult.v.ival = MIN(val2, val1);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Return maximum of 2 vals
WZ_DECL_UNUSED static BOOL scrMax(void)
{
	SDWORD				val1,val2;

	if (!stackPopParams(2, VAL_INT, &val1, VAL_INT, &val2))
	{
		return false;
	}

	scrFunctionResult.v.ival = MAX(val1, val2);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrFMin(void)
{
	float				fval1,fval2;

	if (!stackPopParams(2, VAL_FLOAT, &fval1, VAL_FLOAT, &fval2))
	{
		return false;
	}

	scrFunctionResult.v.fval = MIN(fval2, fval1);
	if (!stackPushResult(VAL_FLOAT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Return maximum of 2 floats
WZ_DECL_UNUSED static BOOL scrFMax(void)
{
	float				fval1,fval2;

	if (!stackPopParams(2, VAL_FLOAT, &fval1, VAL_FLOAT, &fval2))
	{
		return false;
	}

	scrFunctionResult.v.fval = MAX(fval1, fval2);
	if (!stackPushResult(VAL_FLOAT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

BOOL ThreatInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs)
{
	UDWORD				i,structType;
	STRUCTURE			*psStruct;
	DROID				*psDroid;

	const int tx = map_coord(rangeX);
	const int ty = map_coord(rangeY);

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[player][i] == ALLIANCE_FORMED) || (i == player))
		{
			continue;
		}

		//check structures
		for(psStruct = apsStructLists[i]; psStruct; psStruct=psStruct->psNext)
		{
			if (psStruct->visible[player] || psStruct->born == 2)	// if can see it or started there
			{
				if(psStruct->status == SS_BUILT)
				{
					structType = psStruct->pStructureType->type;

					switch(structType)		//dangerous to get near these structures
					{
						case REF_DEFENSE:
						case REF_CYBORG_FACTORY:
						case REF_FACTORY:
						case REF_VTOL_FACTORY:
						case REF_REARM_PAD:

						if (range < 0
						 || world_coord(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y))) < range)	//enemy in range
						{
							return true;
						}

						break;
					}
				}
			}
		}

		//check droids
		for(psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if(psDroid->visible[player])		//can see this droid?
			{
				if (!objHasWeapon((BASE_OBJECT *)psDroid))
				{
					continue;
				}

				//if VTOLs are excluded, skip them
				if(!bVTOLs && ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT) || (psDroid->droidType == DROID_TRANSPORTER)))
				{
					continue;
				}

				if (range < 0
				 || world_coord(hypotf(tx - map_coord(psDroid->pos.x), ty - map_coord(psDroid->pos.y))) < range)	//enemy in range
				{
					return true;
				}
			}
		}
	}

	return false;
}

/// find unrevealed tile closest to pwLooker within the range of wRange
static int scrFogTileInRange(lua_State *L)
{
	SDWORD		tBestX,tBestY, tRangeX, tRangeY;
	UDWORD		tx,ty,i,j,wDist,wBestDist;
	MAPTILE		*psTile;
	BOOL		ok = false;
	
	int pwLookerX   = luaL_checkinteger(L, 3);
	int pwLookerY   = luaL_checkinteger(L, 4);
	int wRangeX     = luaL_checkinteger(L, 5);
	int wRangeY     = luaL_checkinteger(L, 6);
	int wRange      = luaL_checkinteger(L, 7);
	int player      = luaWZ_checkplayer(L, 8);
	int threadRange = luaL_checkinteger(L, 8);

	//Check coords
	if(		pwLookerX < 0
		||	pwLookerX > world_coord(mapWidth)
		||	pwLookerY < 0
		||	pwLookerY > world_coord(mapHeight))
	{
		debug(LOG_ERROR, "scrFogTileInRange: coords off map");
		return false;
	}

	tRangeX = map_coord(wRangeX);				//cache to tile coords, for faster calculations
	tRangeY = map_coord(wRangeY);

	tx = map_coord(pwLookerX);					// change to tile coords.
	ty = map_coord(pwLookerY);

	wBestDist = 99999;
	tBestX = -1; tBestY = -1;

	for(i=0; i<mapWidth;i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			psTile = mapTile(i,j);
		   	if(!TEST_TILE_VISIBLE(player,psTile))	//not vis
		  	{
				//within base range
				if (wRange <= 0
				 || world_coord(iHypot(tRangeX - i, tRangeY - j)) < wRange)		//dist in world units between baseX/baseY and the tile
				{
					//calc dist between this tile and looker
					wDist = world_coord(iHypot(tx - i, ty - j));

					//closer than last one?
					if(wDist < wBestDist)
					{
						//tmpX = i;
						//tmpY = j;
						//if(pickATileGen(&tmpX, &tmpY, 4,zonedPAT))	//can reach (don't need many passes)
						if(zonedPAT(i,j))	//Can reach this tile
						{
							//if((tmpX == i) && (tmpY == j))	//can't allow to change coords, otherwise might send to the same unrevealed tile next time
															//and units will stuck forever
							//{
							if((threadRange <= 0) || (!ThreatInRange(player, threadRange, world_coord(i), world_coord(j), false)))
							{
									wBestDist = wDist;
									tBestX = i;
									tBestY = j;
									ok = true;
							}
							//}
						}
					}
				}
		  	}
		}
	}

	if(ok)	//something found
	{
		lua_pushboolean(L, true);
		lua_pushnumber(L, world_coord(tBestX));
		lua_pushnumber(L, world_coord(tBestY));
		return 3;
	}
	
	lua_pushboolean(L, false);
	return 1;
}

static int scrMapRevealedInRange(lua_State *L)
{
	int tRangeX,tRangeY,tRange;
	int i,j;
	
	int wRangeX = luaL_checkinteger(L, 1);
	int wRangeY = luaL_checkinteger(L, 2);
	int wRange  = luaL_checkinteger(L, 3);
	int player  = luaWZ_checkplayer(L, 4);
	
	//Check coords
	if (wRangeX < 0
	 || wRangeX > world_coord(mapWidth)
	 || wRangeY < 0
	 || wRangeY > world_coord(mapHeight))
	{
		debug(LOG_ERROR,  "scrMapRevealedInRange: coords off map");
		return false;
	}

	// convert to tile coords
	tRange = map_coord(wRange);
	tRangeX = map_coord(wRangeX);				//cache to tile coords, for faster calculations
	tRangeY = map_coord(wRangeY);

	for(i=0; i<mapWidth;i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			// don't bother checking if out of range
			if(abs(tRangeX-i) < tRange && abs(tRangeY-j) < tRange)
			{
				//within range
				if (world_coord(iHypot(tRangeX - i, tRangeY - j)) < wRange  //dist in world units between x/y and the tile
				 && TEST_TILE_VISIBLE(player, mapTile(i, j)))		//not visible
				{
					lua_pushboolean(L, true);
					return 1;
		  		}
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

/// Returns true if a certain map tile was revealed, ie fog of war was removed
static int scrMapTileVisible(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int tileX  = luaL_checkint(L, 2);
	int tileY  = luaL_checkint(L, 3);
	
	//Check coords
	if (tileX < 0
	 || tileX > world_coord(mapWidth)
	 || tileY < 0
	 || tileY > world_coord(mapHeight))
	{
		debug(LOG_ERROR,  "scrMapTileVisible: coords off map");
		return false;
	}

	lua_pushboolean(L, TEST_TILE_VISIBLE( player,mapTile(tileX,tileY) ));
	return 1;
}

//return number of reserach topics that are left to be researched
//for a certain technology to become available
WZ_DECL_UNUSED static BOOL scrNumResearchLeft(void)
{
	RESEARCH			*psResearch;
	SDWORD				player,iResult;
	UWORD				cur,index,tempIndex;
	SWORD				top;

	UWORD				Stack[400];

	BOOL				found;
	PLAYER_RESEARCH		*pPlayerRes;


	if (!stackPopParams(2, VAL_INT, &player, ST_RESEARCH, &psResearch ))
	{
		debug(LOG_ERROR,  "scrNumResearchLeft(): stack failed");
		return false;
	}

	if(psResearch == NULL)
	{
		ASSERT( false, "scrNumResearchLeft(): no such research topic" );
		return false;
	}

	pPlayerRes = asPlayerResList[player];
	index = psResearch - asResearch;	//TODO: fix if needed

	if (index >= numResearch)
	{
		ASSERT( false, "scrNumResearchLeft(): invalid research index" );
		return false;
	}

	found = false;

	if(beingResearchedByAlly(index, player))
	{
		iResult = 1;
	}
	else if(IsResearchCompleted(&pPlayerRes[index]))
	{
		iResult = 0;
	}
	else if(IsResearchStarted(&pPlayerRes[index]))
	{
		iResult = 1;
	}
	else if(IsResearchPossible(&pPlayerRes[index]) || IsResearchCancelled(&pPlayerRes[index]))
	{
		iResult = 1;
	}
	else if(skTopicAvail(index,player))
	{
		iResult = 1;
	}
	else
	{
		iResult = 1;		//init, count the top research topic as 1
		top = -1;

		cur = 0;				//start with first index's PR
		tempIndex = -1;
		while(true)			//do
		{
			if(cur >= asResearch[index].numPRRequired)		//this one has no PRs or end of PRs reached
			{
				top = top - 2;
				if(top < (-1))
				{
					break;		//end of stack
				}
				index = Stack[top + 2];	//if index = -1, then exit
				cur = Stack[top + 1];		//go to next PR of the last node

			}
			else		//end of PRs not reached
			{
				iResult += asResearch[index].numPRRequired;		//add num of PRs this topic has

				tempIndex = asResearch[index].pPRList[cur];		//get cur node's index

				//decide if has to check its PRs
				if(!IsResearchCompleted(&pPlayerRes[tempIndex]) &&	//don't touch if completed already
					!skTopicAvail(index,player) &&					//has no unresearched PRs left if available
					!beingResearchedByAlly(index, player))			//will become available soon anyway
				{
					if(asResearch[tempIndex].numPRRequired > 0)	//node has any nodes itself
					{
						Stack[top+1] = cur;								//so can go back to it further
						Stack[top+2] = index;
						top = top + 2;

						index = tempIndex;		//go 1 level further
						cur = -1;									//start with first PR of this PR next time
					}
				}
			}

			cur++;				//try next node of the main node
			if((cur >= asResearch[index].numPRRequired) && (top <= (-1)))	//nothing left
			{
				break;
			}

		}
	}

	scrFunctionResult.v.ival = iResult;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

//check if any of the ally is researching this topic
BOOL beingResearchedByAlly(SDWORD resIndex, SDWORD player)
{
	STRUCTURE *psOtherStruct;
	SDWORD	i;
	BASE_STATS *Stat;

	Stat = (BASE_STATS*)(asResearch + resIndex);

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(i != player && aiCheckAlliances(player,i))
		{
			//check each research facility to see if they are doing this topic.
			for(psOtherStruct=apsStructLists[i];psOtherStruct;psOtherStruct=psOtherStruct->psNext)
			{
				if(   psOtherStruct->pStructureType->type == REF_RESEARCH
						&& psOtherStruct->status == SS_BUILT
						&& ((RESEARCH_FACILITY *)psOtherStruct->pFunctionality)->psSubject
						 )
				{

					if(((RESEARCH_FACILITY *)psOtherStruct->pFunctionality)->psSubject->ref == Stat->ref)
					{
						return true;
					}
				}
			}

		}
	}

	return false;
}

/// true if player has completed this research
static int scrResearchFinished(lua_State *L)
{
	int index;
	PLAYER_RESEARCH *pPlayerRes;
	
	const char *researchName = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	RESEARCH *psResearch = getResearch(researchName, false);

	if(psResearch == NULL)
	{
		return luaL_error(L, "no such research topic: %s", researchName);
	}

	pPlayerRes = asPlayerResList[player];
	index = psResearch - asResearch;	//TODO: fix if needed

	if (index >= numResearch)
	{
		debug( LOG_ERROR, "invalid research index" );
		return luaL_error(L, "internal error");
	}

	lua_pushboolean(L, IsResearchCompleted(&pPlayerRes[index]));
	return 1;
}

/// true if player has already started researching it
static int scrResearchStarted(lua_State *L)
{
	int index;
	PLAYER_RESEARCH *pPlayerRes;
	
	const char *researchName = luaL_checkstring(L, 1);
	int player = luaWZ_checkplayer(L, 2);
	
	RESEARCH *psResearch = getResearch(researchName, false);

	if(psResearch == NULL)
	{
		return luaL_error(L, "no such research topic: %s", researchName );
	}

	pPlayerRes = asPlayerResList[player];
	index = psResearch - asResearch;	//TODO: fix if needed

	if (index >= numResearch)
	{
		debug( LOG_ERROR, "invalid research index" );
		return luaL_error(L, "internal error");
	}

	lua_pushboolean(L, IsResearchStarted(&pPlayerRes[index]));
	return 1;
}

/// returns true if location is dangerous
static int scrThreatInRange(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int rangeX = luaL_checkinteger(L, 2);
	int rangeY = luaL_checkinteger(L, 3);
	int range  = luaL_checkinteger(L, 4);
	BOOL bVTOLs = luaL_checkboolean(L, 5);

	lua_pushboolean(L, ThreatInRange(player, range, rangeX, rangeY, bVTOLs));
	return 1;
}

/* Calculates the total cost of enemy weapon objects in a certain area */
WZ_DECL_UNUSED static BOOL scrEnemyWeapObjCostInRange(void)
{
	SDWORD				lookingPlayer,range,rangeX,rangeY,i;
	UDWORD				enemyCost = 0;
	BOOL				bVTOLs,bFinished;

	if (!stackPopParams(6, VAL_INT, &lookingPlayer, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR,  "scrEnemyWeapObjCostInRange(): stack failed");
		return false;
	}

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[lookingPlayer][i] == ALLIANCE_FORMED) || (i == lookingPlayer))	//skip allies and myself
		{
			continue;
		}

		enemyCost += playerWeapDroidsCostInRange(i, lookingPlayer, range, rangeX, rangeY, bVTOLs);
		enemyCost += playerWeapStructsCostInRange(i, lookingPlayer, range, rangeX, rangeY, bFinished);
	}

	scrFunctionResult.v.ival = enemyCost;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrEnemyWeapObjCostInRange(): failed to push result");
		return false;
	}

	return true;
}

/* Calculates the total cost of ally (+ looking player)
 * weapon objects in a certain area
 */
WZ_DECL_UNUSED static BOOL scrFriendlyWeapObjCostInRange(void)
{
	SDWORD				player,range,rangeX,rangeY,i;
	UDWORD				friendlyCost = 0;
	BOOL				bVTOLs,bFinished;

	if (!stackPopParams(6, VAL_INT, &player, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR,  "scrFriendlyWeapObjCostInRange(): stack failed");
		return false;
	}

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[player][i] == ALLIANCE_FORMED) || (i == player))	//skip enemies
		{
			friendlyCost += numPlayerWeapDroidsInRange(i, player, range, rangeX, rangeY, bVTOLs);
			friendlyCost += numPlayerWeapStructsInRange(i, player, range, rangeX, rangeY,bFinished);
		}
	}

	scrFunctionResult.v.ival = friendlyCost;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/**
 * Helper function for numPlayerWeapDroidsInRange and playerWeapDroidsCostInRange.
 * Will either count the number of droids or calculate the total costs.
 */
static UDWORD costOrAmountInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range,
								   SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs, BOOL justCount)
{
	UDWORD				droidCost;
	DROID				*psDroid;

	droidCost = 0;

	//check droids
	for(psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->visible[lookingPlayer])		//can see this droid?
		{
			if (!objHasWeapon((BASE_OBJECT *)psDroid))
			{
				continue;
			}

			//if VTOLs are excluded, skip them
			if(!bVTOLs && ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT) || (psDroid->droidType == DROID_TRANSPORTER)))
			{
				continue;
			}

			if (range < 0
			 || iHypot(rangeX - psDroid->pos.x, rangeY - psDroid->pos.y) < range)  //enemy in range
			{
				if (justCount)
				{
					droidCost++;
				}
				else
				{
					droidCost += calcDroidPower(psDroid);
				}
			}
		}
	}

	return droidCost;
}

UDWORD numPlayerWeapDroidsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs)
{
	return costOrAmountInRange(player, lookingPlayer, range, rangeX, rangeY, bVTOLs, true /*only count*/);
}

UDWORD playerWeapDroidsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range,
								   SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs)
{
	return costOrAmountInRange(player, lookingPlayer, range, rangeX, rangeY, bVTOLs, false /*total cost*/);
}



UDWORD numPlayerWeapStructsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range,
								   SDWORD rangeX, SDWORD rangeY, BOOL bFinished)
{
	const STRUCTURE* psStruct;

	const int tx = map_coord(rangeX);
	const int ty = map_coord(rangeY);

	unsigned int numStructs = 0;

	//check structures
	for (psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->visible[lookingPlayer]	//if can see it
		 && objHasWeapon((BASE_OBJECT *) psStruct)) // check whether this structure is "dangerous"
		{
			if (!bFinished || psStruct->status == SS_BUILT)
			{
				if (range < 0
				 || world_coord(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y))) < range)	//enemy in range
				{
					++numStructs;
				}
			}
		}
	}

	return numStructs;
}

UDWORD playerWeapStructsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range,
								   SDWORD rangeX, SDWORD rangeY, BOOL bFinished)
{
	const STRUCTURE* psStruct;

	unsigned int structsCost = 0;

	//check structures
	for (psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->visible[lookingPlayer]	//if can see it
		 && objHasWeapon((BASE_OBJECT *) psStruct))
		{
			if (!bFinished
			 || psStruct->status == SS_BUILT)
			{
				if (range < 0
				 || world_coord(hypotf(map_coord(rangeX) - map_coord(psStruct->pos.x), map_coord(rangeY) - map_coord(psStruct->pos.y))) < range)	//enemy in range
				{
					structsCost += structPowerToBuild(psStruct);
				}
			}
		}
	}

	return structsCost;
}

static int scrNumPlayerWeapDroidsInRange(lua_State *L)
{
	int targetPlayer  = luaWZ_checkplayer(L, 1);
	int lookingPlayer = luaWZ_checkplayer(L, 2);
	int rangeX        = luaL_checkinteger(L, 3);
	int rangeY        = luaL_checkinteger(L, 4);
	int range         = luaL_checkinteger(L, 5);
	BOOL bVTOLs       = luaL_checkboolean(L, 6);

	lua_pushinteger(L, numPlayerWeapDroidsInRange(targetPlayer, lookingPlayer, range, rangeX, rangeY, bVTOLs));
	return 1;
}

static int scrNumPlayerWeapStructsInRange(lua_State *L)
{
	int targetPlayer  = luaWZ_checkplayer(L, 1);
	int lookingPlayer = luaWZ_checkplayer(L, 2);
	int rangeX        = luaL_checkint(L, 3);
	int rangeY        = luaL_checkint(L, 4);
	int range         = luaL_checkint(L, 5);
	BOOL bFinished    = luaL_checkboolean(L, 6);

	lua_pushinteger(L, numPlayerWeapStructsInRange(targetPlayer, lookingPlayer, range, rangeX, rangeY, bFinished));
	return 1;
}

WZ_DECL_UNUSED static BOOL scrNumEnemyObjInRange(void)
{
	SDWORD				lookingPlayer,range,rangeX,rangeY;
	BOOL				bVTOLs,bFinished;

	if (!stackPopParams(6, VAL_INT, &lookingPlayer, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR, "scrNumEnemyObjInRange(): stack failed");
		return false;
	}

	scrFunctionResult.v.ival = numEnemyObjInRange(lookingPlayer, range, rangeX, rangeY, bVTOLs, bFinished);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumEnemyObjInRange(): failed to push result");
		return false;
	}

	return true;
}

UDWORD numEnemyObjInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY,
						  BOOL bVTOLs, BOOL bFinished)
{
	unsigned int i;
	const STRUCTURE* psStruct;
	const DROID* psDroid;

	const int tx = map_coord(rangeX);
	const int ty = map_coord(rangeY);

	unsigned int numEnemies = 0;

	for (i = 0; i < MAX_PLAYERS;i++)
	{
		if (alliances[player][i] == ALLIANCE_FORMED
		 || i == player)
		{
			continue;
		}

		//check structures
		for (psStruct = apsStructLists[i]; psStruct; psStruct = psStruct->psNext)
		{
			if (psStruct->visible[player])	//if can see it
			{
				if (!bFinished
				 || psStruct->status == SS_BUILT)
				{
					if (range < 0
					 || world_coord(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y))) < range)	//enemy in range
					{
						numEnemies++;
					}
				}
			}
		}

		//check droids
		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->visible[player])		//can see this droid?
			{
				//if VTOLs are excluded, skip them
				if (!bVTOLs
				 && (asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT
				  || psDroid->droidType == DROID_TRANSPORTER))
				{
					continue;
				}

				if (range < 0
				 || world_coord(hypotf(tx - map_coord(psDroid->pos.x), ty - map_coord(psDroid->pos.y))) < range)	//enemy in range
				{
					numEnemies++;
				}
			}
		}
	}

	return numEnemies;
}

/* Similar to structureBuiltInRange(), but also returns true if structure is not finished */
static int scrNumStructsByStatInRange(lua_State *L)
{
	int index = luaWZObj_checkstructurestat(L, 1);
	Vector2i coord = luaWZ_checkWorldCoords(L, 2);
	int range = luaL_checkint(L, 4);
	int lookingPlayer = luaWZ_checkplayer(L, 5);
	int player = luaWZ_checkplayer(L, 6);

	STRUCTURE_STATS *psTarget = &asStructureStats[index];
	int rangeSquared, NumStruct, xdiff, ydiff;
	STRUCTURE *psCurr;

	NumStruct = 0;

	//now look through the players list of structures to see if this type
	//exists within range
	rangeSquared = range * range;
	for(psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		xdiff = (SDWORD)psCurr->pos.x - coord.x;
		ydiff = (SDWORD)psCurr->pos.y - coord.y;
		if (xdiff*xdiff + ydiff*ydiff <= rangeSquared)
		{
			if( strcmp(psCurr->pStructureType->pName,psTarget->pName) == 0 )
			{
				if(psCurr->visible[lookingPlayer])		//can we see it?
				{
					NumStruct++;
				}
			}
		}
	}

	lua_pushinteger(L, NumStruct);
	return 1;
}

WZ_DECL_UNUSED static BOOL scrNumStructsByStatInArea(void)
{
	SDWORD		player, lookingPlayer, index, x1, y1, x2, y2;
	SDWORD		NumStruct;
	STRUCTURE	*psCurr;

	STRUCTURE_STATS		*psStats;

	if (!stackPopParams(7, ST_STRUCTURESTAT, &index, VAL_INT, &x1, VAL_INT, &y1,
		VAL_INT, &x2, VAL_INT, &y2, VAL_INT, &lookingPlayer, VAL_INT, &player))
	{
		debug(LOG_ERROR,"scrNumStructsByStatInArea: failed to pop");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR,"scrNumStructsByStatInArea: player number too high");
		ASSERT( false, "scrStructureBuiltInRange:player number is too high" );
		return false;
	}


	if (index < (SDWORD)0 || index > (SDWORD)numStructureStats)
	{
		debug(LOG_ERROR, "scrNumStructsByStatInArea: invalid structure stat");
		ASSERT( false, "scrStructureBuiltInRange : Invalid structure stat" );
		return false;
	}

	ASSERT_OR_RETURN( false, index < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", index, numStructureStats);
	psStats = (STRUCTURE_STATS *)(asStructureStats + index);

	ASSERT_OR_RETURN( false, psStats != NULL, "Invalid structure pointer" );

	NumStruct = 0;

	for (psCurr = apsStructLists[player]; psCurr != NULL;
		psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType == psStats)
		{
			if(psCurr->visible[lookingPlayer])		//can we see it?
			{
				if(psCurr->pos.x < x1) continue;		//not in bounds
				if(psCurr->pos.y < y1) continue;		//not in bounds
				if(psCurr->pos.x > x2) continue;		//not in bounds
				if(psCurr->pos.y > y2) continue;		//not in bounds
				NumStruct++;
			}
		}
	}

	scrFunctionResult.v.ival = NumStruct;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

//returns num of visible structures of a certain player in range (only visible ones)
WZ_DECL_UNUSED static BOOL scrNumStructsButNotWallsInRangeVis(void)
{
	SDWORD		player, lookingPlayer, x, y, range;
	SDWORD		rangeSquared,NumStruct;
	STRUCTURE	*psCurr;
	SDWORD		xdiff, ydiff;

	if (!stackPopParams(5, VAL_INT, &x, VAL_INT, &y,
		VAL_INT, &range, VAL_INT, &lookingPlayer, VAL_INT, &player))
	{
		debug(LOG_ERROR,"scrNumStructsButNotWallsInRangeVis: failed to pop");
		return false;
	}

	if ((player >= MAX_PLAYERS) || (lookingPlayer >= MAX_PLAYERS))
	{
		ASSERT( false, "scrNumStructsButNotWallsInRangeVis:player number is too high" );
		return false;
	}

	if (x < 0
	 || map_coord(x) > (SDWORD)mapWidth)
	{
		ASSERT( false, "scrNumStructsButNotWallsInRangeVis : invalid X coord" );
		return false;
	}
	if (y < 0
	 || map_coord(y) > (SDWORD)mapHeight)
	{
		ASSERT( false,"scrNumStructsButNotWallsInRangeVis : invalid Y coord" );
		return false;
	}
	if (range < (SDWORD)0)
	{
		ASSERT( false, "scrNumStructsButNotWallsInRangeVis : Rnage is less than zero" );
		return false;
	}

	NumStruct = 0;

	//now look through the players list of structures
	rangeSquared = range * range;
	for(psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		if ((psCurr->pStructureType->type != REF_WALL) &&
		(psCurr->pStructureType->type != REF_WALLCORNER))
		{
			if(psCurr->visible[lookingPlayer])		//can we see it?
			{
				xdiff = (SDWORD)psCurr->pos.x - x;
				ydiff = (SDWORD)psCurr->pos.y - y;
				if (xdiff*xdiff + ydiff*ydiff <= rangeSquared)
				{
					NumStruct++;
				}
			}
		}
	}

	scrFunctionResult.v.ival = NumStruct;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/// Only returns structure if it is visible
static int scrGetStructureVis(lua_State *L)
{
	STRUCTURE			*psStruct;
	UDWORD				structType;
	BOOL				found;
	
	int index         = luaWZObj_checkstructurestat(L, 1);
	int player        = luaWZ_checkplayer(L, 2);
	int lookingPlayer = luaWZ_checkplayer(L, 3);
	
	structType = asStructureStats[index].ref;

	//search the players' list of built structures to see if one exists
	found = false;
	for (psStruct = apsStructLists[player]; psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->pStructureType->ref == structType)
		{
			if(psStruct->visible[lookingPlayer])
			{
				found = true;
				break;
			}
		}
	}

	//make sure pass NULL back if not got one
	if (!found)
	{
		lua_pushnil(L);
		return 1;
	}
	
	luaWZObj_pushstructure(L, psStruct);
	return 1;
}

static int scrChooseValidLoc(lua_State *L)
{
	UDWORD tx,ty;

	int x             = luaL_checkint(L, 1);
	int y             = luaL_checkint(L, 2);
	int player        = luaWZ_checkplayer(L, 3);
	int threatRange   = luaL_checkint(L, 4);

	//Check coords
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		return luaL_error(L, "coords off map");
	}

	tx = map_coord(x);
	ty = map_coord(y);

	if(pickATileGenThreat(&tx, &ty, LOOK_FOR_EMPTY_TILE, threatRange, player, zonedPAT))
	{
		lua_pushinteger(L, world_coord(tx));
		lua_pushinteger(L, world_coord(ty));
		lua_pushboolean(L, true);
		return 3;
	}
	else
	{
		lua_pushboolean(L, false);
		return 1;
	}
}

//returns closest enemy object
WZ_DECL_UNUSED static BOOL scrGetClosestEnemy(void)
{
	SDWORD				x,y,tx,ty, player, range,i;
	UDWORD				dist, bestDist;
	BOOL				weaponOnly, bVTOLs, bFound = false;	//only military objects?
	BASE_OBJECT			*psObj = NULL;
	STRUCTURE			*psStruct = NULL;
	DROID				*psDroid = NULL;

	if (!stackPopParams(6, VAL_INT, &x, VAL_INT, &y,
		 VAL_INT, &range,  VAL_BOOL, &weaponOnly, VAL_BOOL, &bVTOLs, VAL_INT, &player))
	{
		debug(LOG_ERROR,"scrGetClosestEnemy: stack failed");
		return false;
	}

	//Check coords
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		debug(LOG_ERROR,"scrGetClosestEnemy: coords off map");
		return false;
	}

	tx = map_coord(x);
	ty = map_coord(y);

	bestDist = 99999;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[player][i] == ALLIANCE_FORMED) || (i == player))
		{
			continue;
		}


		//check droids
		for(psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if(psDroid->visible[player])		//can see this droid?
			{
				//if only weapon droids and don't have it, then skip
				if (weaponOnly && !objHasWeapon((BASE_OBJECT *)psDroid))
				{
					continue;
				}

				//if VTOLs are excluded, skip them
				if(!bVTOLs && ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT) || (psDroid->droidType == DROID_TRANSPORTER)))
				{
					continue;
				}

				dist = world_coord(hypotf(tx - map_coord(psDroid->pos.x), ty - map_coord(psDroid->pos.y)));
				if(dist < bestDist)
				{
					if((range < 0) || (dist < range))	//enemy in range
					{
						bestDist = dist;
						bFound = true;
						psObj = (BASE_OBJECT*)psDroid;
					}
				}
			}
		}


		//check structures
		for(psStruct = apsStructLists[i]; psStruct; psStruct=psStruct->psNext)
		{
			if(psStruct->visible[player])	//if can see it
			{
				//only need defenses?
				if(weaponOnly && (!objHasWeapon((BASE_OBJECT *) psStruct) || (psStruct->status != SS_BUILT) ))	//non-weapon-structures	or not finished
				{
					continue;
				}

				dist = world_coord(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y)));
				if(dist < bestDist)
				{
					if((range < 0) || (dist < range))	//in range
					{
						bestDist = dist;
						bFound = true;
						psObj = (BASE_OBJECT*)psStruct;
					}
				}
			}
		}

	}

	if(bFound)
	{
		scrFunctionResult.v.oval = psObj;
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))
		{
			return false;
		}
	}
	else
	{
		scrFunctionResult.v.oval = NULL;
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))
		{
			return false;
		}
	}

	return true;
}

//How many droids can it still fit?
WZ_DECL_UNUSED static BOOL scrTransporterCapacity(void)
{
	DROID			*psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		debug(LOG_ERROR, "scrTransporterCapacity(): failed to pop params");
		return false;
	}

	if(psDroid == NULL)
	{
		debug(LOG_ERROR,"scrTransporterCapacity(): NULLOBJECT passed");
		return false;
	}

	if(psDroid->droidType != DROID_TRANSPORTER)
	{
		debug(LOG_ERROR, "scrTransporterCapacity(): passed droid is not a transporter");
		return false;
	}

	scrFunctionResult.v.ival = calcRemainingCapacity(psDroid);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrHasIndirectWeapon(): failed to push result");
		return false;
	}

	return true;
}

//is it?
WZ_DECL_UNUSED static BOOL scrTransporterFlying(void)
{
	DROID			*psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		debug(LOG_ERROR, "scrTransporterFlying(): failed to pop params");
		return false;
	}

	if(psDroid == NULL)
	{
		debug(LOG_ERROR,"scrTransporterFlying(): NULLOBJECT passed");
		return false;
	}

	if(psDroid->droidType != DROID_TRANSPORTER)
	{
		debug(LOG_ERROR,"scrTransporterFlying(): passed droid is not a transporter");
		return false;
	}

	scrFunctionResult.v.bval = transporterFlying(psDroid);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR,"scrTransporterFlying(): failed to push result");
		return false;
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrUnloadTransporter(void)
{
	DROID			*psDroid;
	SDWORD			x,y;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &x, VAL_INT, &y))
	{
		debug(LOG_ERROR,"scrUnloadTransporter(): failed to pop params");
		return false;
	}

	if(psDroid == NULL)
	{
		debug(LOG_ERROR,"scrUnloadTransporter(): NULLOBJECT passed");
		return false;
	}

	if(psDroid->droidType != DROID_TRANSPORTER)
	{
		debug(LOG_ERROR,"scrUnloadTransporter(): passed droid is not a transporter");
		return false;
	}

	unloadTransporter(psDroid,x,y, false);

	return true;
}

/// return true if droid is a member of any group
static int scrHasGroup(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);

	lua_pushboolean(L, psDroid->psGroup != NULL);
	return 1;
}

/* Range is in world units! */
WZ_DECL_UNUSED static BOOL scrObjWeaponMaxRange(void)
{
	BASE_OBJECT			*psObj;
	WEAPON_STATS		*psStats;
	DROID				*psDroid;
	STRUCTURE			*psStruct;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrObjWeaponMaxRange: stack failed");
		return false;
	}

	//check if valid type
	if(psObj->type == OBJ_DROID)
	{
		psDroid = (DROID*)psObj;
		if (psDroid->asWeaps[0].nStat != 0)
		{
			ASSERT_OR_RETURN(false, psDroid->asWeaps[0].nStat < numWeaponStats, "Invalid range referenced.");
			psStats = asWeaponStats + psDroid->asWeaps[0].nStat;
			scrFunctionResult.v.ival = psStats->longRange;
			if (!stackPushResult(VAL_INT, &scrFunctionResult))
			{
				return false;
			}

			return true;
		}
	}
	else if(psObj->type == OBJ_STRUCTURE)
	{
		psStruct = (STRUCTURE*)psObj;
		if (psStruct->asWeaps[0].nStat != 0)
		{
			ASSERT_OR_RETURN(false, psStruct->asWeaps[0].nStat < numWeaponStats, "Invalid range referenced.");
			psStats = asWeaponStats + psStruct->asWeaps[0].nStat;
			scrFunctionResult.v.ival = psStats->longRange;
			if (!stackPushResult(VAL_INT, &scrFunctionResult))
			{
				return false;
			}

			return true;
		}
	}

	scrFunctionResult.v.ival = 0;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR,"scrObjWeaponMaxRange: wrong object type");
		return false;
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrObjHasWeapon(void)
{
	BASE_OBJECT			*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrObjHasWeapon: stack failed");
		return false;
	}

	//check if valid type
	if(objHasWeapon(psObj))
	{
		scrFunctionResult.v.bval = true;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			return false;
		}

		return true;
	}


	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrObjectHasIndirectWeapon(void)
{
	WEAPON_STATS	*psWeapStats;
	BOOL			bIndirect;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrHasIndirectWeapon(): failed to pop params");
		return false;
	}

	if (psObj == NULL)
	{
		debug(LOG_ERROR,"scrHasIndirectWeapon(): NULLOBJECT passed");
		return false;
	}

	bIndirect = false;
	if(psObj->type == OBJ_DROID)
	{
		if (((DROID *)psObj)->asWeaps[0].nStat > 0)
		{
			psWeapStats = asWeaponStats + ((DROID *)psObj)->asWeaps[0].nStat;
			bIndirect = !proj_Direct(psWeapStats);
		}
	}
	else if(psObj->type == OBJ_STRUCTURE)
	{
		if (((STRUCTURE *)psObj)->asWeaps[0].nStat > 0)
		{
			psWeapStats = asWeaponStats + ((STRUCTURE *)psObj)->asWeaps[0].nStat;
			bIndirect = !proj_Direct(psWeapStats);
		}
	}

	scrFunctionResult.v.bval = bIndirect;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR,"scrHasIndirectWeapon(): failed to push result");
		return false;
	}

	return true;
}

//returns closest droid by type
WZ_DECL_UNUSED static BOOL scrGetClosestEnemyDroidByType(void)
{
	SDWORD				x,y,tx,ty, player, range,i,type;
	UDWORD				dist,bestDist;
	BOOL				bFound = false;	//only military objects?
	BOOL				bVTOLs;
	DROID				*psDroid = NULL, *foundDroid = NULL;

	if (!stackPopParams(6, VAL_INT, &x, VAL_INT, &y,
		 VAL_INT, &range,  VAL_INT, &type, VAL_BOOL, &bVTOLs, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrGetClosestEnemyDroidByType: stack failed");
		return false;
	}

	//Check coords
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		debug(LOG_ERROR,"scrGetClosestEnemyDroidByType: coords off map");
		return false;
	}

	tx = map_coord(x);
	ty = map_coord(y);

	bestDist = 99999;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[player][i] == ALLIANCE_FORMED) || (i == player))
		{
			continue;
		}

		//check droids
		for(psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			//if VTOLs are excluded, skip them (don't check for transporter this time)
			if(!bVTOLs && (asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT) )
			{
				continue;
			}

			if(psDroid->visible[player])		//can see this droid?
			{
				//skip?
				if ((type != (-1)) && (psDroid->droidType != type))
				{
					continue;
				}

				dist = world_coord(hypotf(tx - map_coord(psDroid->pos.x), ty - map_coord(psDroid->pos.y)));
				if(dist < bestDist)
				{
					if(dist < range)	//enemy in range
					{
						bestDist = dist;
						bFound = true;
						foundDroid = psDroid;
					}
				}
			}
		}
	}

	if(bFound)
	{
		scrFunctionResult.v.oval = foundDroid;
		if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
		{
			return false;
		}
	}
	else
	{
		scrFunctionResult.v.oval = NULL;
		if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
		{
			return false;
		}
	}

	return true;
}

/// returns closest structure by type
static int scrGetClosestEnemyStructByType(lua_State *L)
{
	SDWORD				tx, ty, i, dist;
	UDWORD				bestDist;
	BOOL				bFound = false;	//only military objects?
	STRUCTURE			*psStruct = NULL, *foundStruct = NULL;
	
	int x      = luaL_checkinteger(L, 1);
	int y      = luaL_checkinteger(L, 2);
	int range  = luaL_checkinteger(L, 3);
	int type   = luaL_checkinteger(L, 4);
	int player = luaWZ_checkplayer(L, 5);

	//Check coords
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		debug(LOG_ERROR,"scrGetClosestEnemyStructByType: coords off map");
		return false;
	}

	tx = map_coord(x);
	ty = map_coord(y);

	bestDist = 99999;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[player][i] == ALLIANCE_FORMED) || (i == player))
		{
			continue;
		}

		//check structures
		for(psStruct = apsStructLists[i]; psStruct; psStruct=psStruct->psNext)
		{
			if(psStruct->visible[player])	//if can see it
			{
				//only need defenses?
				if((type != (-1)) && (psStruct->pStructureType->type != type))	//non-weapon-structures
				{
					continue;
				}

				dist = world_coord(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y)));
				if(dist < bestDist)
				{
					if((range < 0) || (dist < range))	//in range or no range check
					{
						bestDist = dist;
						bFound = true;
						foundStruct = psStruct;
					}
				}
			}
		}
	}
	
	if (!bFound)
	{
		lua_pushnil(L);
		return 1;
	}
	
	luaWZObj_pushstructure(L, foundStruct);
	return 1;
}



//Approx point of intersection of a circle and a line with start loc being circle's center point
static int scrCirclePerimPoint(lua_State *L)
{
	float factor, deltaX, deltaY;

	int basex    = luaL_checkint(L, 1);
	int basey    = luaL_checkint(L, 2);
	int x        = luaL_checkint(L, 3);
	int y        = luaL_checkint(L, 4);
	float radius = luaL_checknumber(L, 5);

	if(radius == 0)
	{
		debug(LOG_ERROR,"scrCirclePerimPoint: radius == 0.");
		return true;
	}

	deltaX = (float)(x - basex);	//x len (signed!)
	deltaY = (float)(y - basey);

	factor =  hypotf(deltaX, deltaY) / radius;			//by what factor is distance > radius?

	//if point was inside of the circle, don't modify passed parameter
	if(factor == 0)
	{
		return luaL_error(L, "point is inside the circle");
	}

	//calc new len
	deltaX = deltaX / factor;
	deltaY = deltaY / factor;

	//now add new len to the center coords
	lua_pushinteger(L, basex + deltaX);
	lua_pushinteger(L, basey + deltaY);
	return 2;
}

//send my vision to AI
WZ_DECL_UNUSED static BOOL scrGiftRadar(void)
{
	SDWORD	playerFrom, playerTo;
	BOOL	playMsg;

	if (!stackPopParams(3, VAL_INT, &playerFrom, VAL_INT, &playerTo, VAL_BOOL, &playMsg))
	{
		debug(LOG_ERROR,"scrGiftRadar(): stack failed");
		return false;
	}

	if (playerFrom >= MAX_PLAYERS || playerTo >= MAX_PLAYERS)
	{
		debug(LOG_ERROR,"scrGiftRadar: player out of range");
		return false;
	}

	giftRadar(playerFrom,playerTo,true);

	if(playMsg)
		audio_QueueTrack(ID_SENSOR_DOWNLOAD);

	return true;
}

WZ_DECL_UNUSED static BOOL scrNumAllies(void)
{
	SDWORD			player,numAllies,i;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrNumAllies: failed to pop");
		return false;
	}

	if (player < 0)
	{
		debug(LOG_ERROR, "scrNumAllies: player < 0");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR,"scrNumAllies: player index too high");
		return false;
	}

	numAllies = 0;
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(i != player)
		{
			if(alliances[i][player] == ALLIANCE_FORMED)
			{
				numAllies++;
			}
		}
	}

	scrFunctionResult.v.ival = numAllies;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


//num aa defenses in range
WZ_DECL_UNUSED static BOOL scrNumAAinRange(void)
{
	SDWORD				targetPlayer,lookingPlayer,range,rangeX,rangeY;
	SDWORD				tx,ty;
	UDWORD				numFound = 0;
	STRUCTURE	*psStruct;

	if (!stackPopParams(5, VAL_INT, &targetPlayer, VAL_INT, &lookingPlayer,
		VAL_INT, &rangeX, VAL_INT, &rangeY, VAL_INT, &range))
	{
		debug(LOG_ERROR,"scrNumAAinRange(): stack failed");
		return false;
	}

	tx = map_coord(rangeX);
	ty = map_coord(rangeY);

	numFound = 0;

	//check structures
	for(psStruct = apsStructLists[targetPlayer]; psStruct; psStruct=psStruct->psNext)
	{
		if(psStruct->visible[lookingPlayer])	//if can see it
		{
			if (objHasWeapon((BASE_OBJECT *) psStruct) && (asWeaponStats[psStruct->asWeaps[0].nStat].surfaceToAir & SHOOT_IN_AIR))
			{
				if (range < 0
				 || world_coord(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y))) < range)	//enemy in range
				{
					numFound++;
				}
			}
		}
	}

	scrFunctionResult.v.ival = numFound;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR,"scrNumAAinRange(): failed to push result");
		return false;
	}

	return true;
}

//select droid
WZ_DECL_UNUSED static BOOL scrSelectDroid(void)
{
	BOOL	bSelect;
	DROID	*psDroid;

	if (!stackPopParams(2, ST_DROID, &psDroid, VAL_BOOL, &bSelect))
	{
		debug(LOG_ERROR, "scrSelectDroid(): stack failed");
		return false;
	}

	if(psDroid == NULL)
	{
		debug(LOG_ERROR,"scrSelectDroid(): droid is NULLOBJECT");
		return false;
	}

	psDroid->selected = bSelect;

	return true;
}

//select droid group
WZ_DECL_UNUSED static BOOL scrSelectGroup(void)
{
	BOOL		bSelect;
	DROID_GROUP	*psGroup;
	DROID		*psCurr;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_BOOL, &bSelect))
	{
		debug(LOG_ERROR, "scrSelectGroup(): stack failed");
		return false;
	}

	for(psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		psCurr->selected = bSelect;
	}

	return true;
}

static int scrPlayerLoaded(lua_State *L)
{
	BOOL			bPlayerHasFactories=false;
	STRUCTURE		*psCurr;
	
	int player = luaWZ_checkplayer(L, 1);

	/* see if there are any player factories left */
	if(apsStructLists[player])
	{
		for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if(StructIsFactory(psCurr))
			{
				bPlayerHasFactories = true;
				break;
			}
		}
	}

	/* player is active if he has at least a unit or some factory */
	lua_pushboolean(L, apsDroidLists[player] != NULL || bPlayerHasFactories);
	return 1;
}


		/********************************/
		/*		AI Experience Stuff		*/
		/********************************/

//Returns enemy base x and y for a certain player
WZ_DECL_UNUSED static BOOL scrLearnPlayerBaseLoc(void)
{
	SDWORD				playerStoring,enemyPlayer, x, y;

	if (!stackPopParams(4, VAL_INT, &playerStoring, VAL_INT, &enemyPlayer,
						VAL_INT, &x, VAL_INT, &y))
	{
		debug(LOG_ERROR, "scrLearnPlayerBaseLoc(): stack failed");
		return false;
	}

	if((playerStoring >= MAX_PLAYERS) || (enemyPlayer >= MAX_PLAYERS))
	{
		debug(LOG_ERROR, "scrLearnPlayerBaseLoc: player index too high.");
		return false;
	}

	if((playerStoring < 0) || (enemyPlayer < 0))
	{
		debug(LOG_ERROR, "scrLearnPlayerBaseLoc: player index too low.");
		return false;
	}

	if (x < 0
	 || x >= world_coord(mapWidth)
	 || y < 0
	 || y >= world_coord(mapHeight))
	{
		debug(LOG_ERROR, "scrLearnPlayerBaseLoc: coords off map");
		return false;
	}

	baseLocation[playerStoring][enemyPlayer][0] = x;
	baseLocation[playerStoring][enemyPlayer][1] = y;

	debug_console("Learned player base.");

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/// Saves enemy base x and y for a certain player
static int scrRecallPlayerBaseLoc(lua_State *L)
{
	int x,y;
	
	int playerStoring = luaWZ_checkplayer(L, 1);
	int enemyPlayer   = luaWZ_checkplayer(L, 2);
	
	if(!CanRememberPlayerBaseLoc(playerStoring, enemyPlayer))		//return false if this one not set yet
	{
		lua_pushboolean(L, false);
		return 1;
	}

	x = baseLocation[playerStoring][enemyPlayer][0];
	y = baseLocation[playerStoring][enemyPlayer][1];

	lua_pushboolean(L, true);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 3;
}

/* Checks if player base loc is stored */
WZ_DECL_UNUSED static BOOL scrCanRememberPlayerBaseLoc(void)
{
	SDWORD				playerStoring,enemyPlayer;

	if (!stackPopParams(2, VAL_INT, &playerStoring, VAL_INT, &enemyPlayer))
	{
		debug(LOG_ERROR, "scrCanRememberPlayerBaseLoc(): stack failed");
		return false;
	}

	if((playerStoring >= MAX_PLAYERS) || (enemyPlayer >= MAX_PLAYERS))
	{
		debug(LOG_ERROR,"scrCanRememberPlayerBaseLoc: player index too high.");
		return false;
	}

	if((playerStoring < 0) || (enemyPlayer < 0))
	{
		debug(LOG_ERROR,"scrCanRememberPlayerBaseLoc: player index too low.");
		return false;
	}

	scrFunctionResult.v.bval = CanRememberPlayerBaseLoc(playerStoring, enemyPlayer);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Stores the place where we were attacked at */
WZ_DECL_UNUSED static BOOL scrLearnBaseDefendLoc(void)
{
	SDWORD				playerStoring,enemyPlayer, x, y;

	if (!stackPopParams(4, VAL_INT, &playerStoring, VAL_INT, &enemyPlayer,
						VAL_INT, &x, VAL_INT, &y))
	{
		debug(LOG_ERROR, "scrLearnBaseDefendLoc(): stack failed");
		return false;
	}

	if((playerStoring >= MAX_PLAYERS) || (enemyPlayer >= MAX_PLAYERS))
	{
		debug(LOG_ERROR,"scrLearnBaseDefendLoc: player index too high.");
		return false;
	}

	if((playerStoring < 0) || (enemyPlayer < 0))
	{
		debug(LOG_ERROR,"scrLearnBaseDefendLoc: player index too low.");
		return false;
	}

	if (x < 0
	 || x >= world_coord(mapWidth)
	 || y < 0
	 || y >= world_coord(mapHeight))
	{
		debug(LOG_ERROR,"scrLearnBaseDefendLoc: coords off map");
		return false;
	}

	StoreBaseDefendLoc(x, y, playerStoring);

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Stores the place where we were attacked at */
WZ_DECL_UNUSED static BOOL scrLearnOilDefendLoc(void)
{
	SDWORD				playerStoring,enemyPlayer, x, y;

	if (!stackPopParams(4, VAL_INT, &playerStoring, VAL_INT, &enemyPlayer,
						VAL_INT, &x, VAL_INT, &y))
	{
		debug(LOG_ERROR, "scrLearnOilDefendLoc(): stack failed");
		return false;
	}

	if((playerStoring >= MAX_PLAYERS) || (enemyPlayer >= MAX_PLAYERS))
	{
		debug(LOG_ERROR,"scrLearnOilDefendLoc: player index too high.");
		return false;
	}

	if((playerStoring < 0) || (enemyPlayer < 0))
	{
		debug(LOG_ERROR,"scrLearnOilDefendLoc: player index too low.");
		return false;
	}

	if (x < 0
	 || x >= world_coord(mapWidth)
	 || y < 0
	 || y >= world_coord(mapHeight))
	{
		debug(LOG_ERROR,"scrLearnOilDefendLoc: coords off map");
		return false;
	}

	StoreOilDefendLoc(x, y, playerStoring);

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/// Returns -1 if this location is not stored yet, otherwise returns index
static int scrGetBaseDefendLocIndex(lua_State *L)
{
	int playerStoring = luaWZ_checkplayer(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);

	if (x < 0
	 || x >= world_coord(mapWidth)
	 || y < 0
	 || y >= world_coord(mapHeight))
	{
		debug(LOG_ERROR, "scrGetBaseDefendLocIndex: coords off map");
		return false;
	}

	lua_pushinteger(L, GetBaseDefendLocIndex(x,y,playerStoring));
	return 1;
}

/// Returns -1 if this location is not stored yet, otherwise returns index
static int scrGetOilDefendLocIndex(lua_State *L)
{
	int playerStoring = luaWZ_checkplayer(L, 1);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);

	if (x < 0
	 || x >= world_coord(mapWidth)
	 || y < 0
	 || y >= world_coord(mapHeight))
	{
		debug(LOG_ERROR, "scrGetOilDefendLocIndex: coords off map");
		return false;
	}

	lua_pushinteger(L, GetOilDefendLocIndex(x,y,playerStoring));
	return 1;
}

/// Returns number of available locations
static int scrGetBaseDefendLocCount(lua_State *L)
{
	lua_pushinteger(L, MAX_BASE_DEFEND_LOCATIONS);
	return 1;
}

/// Returns number of available locations
static int scrGetOilDefendLocCount(lua_State *L)
{
	lua_pushinteger(L, MAX_OIL_DEFEND_LOCATIONS);
	return 1;
}

/// Returns a locations and its priority
static int scrRecallBaseDefendLoc(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int index = luaL_checkint(L, 2);

	if(index < 0 || index >= MAX_BASE_DEFEND_LOCATIONS)
	{
		return luaL_argerror(L, 2, "wrong index");
	}

	//check if can recall at this location
	if(!CanRememberPlayerBaseDefenseLoc(player, index))
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	lua_pushinteger(L, baseDefendLocation[player][index][0]);
	lua_pushinteger(L, baseDefendLocation[player][index][1]);
	lua_pushinteger(L, baseDefendLocPrior[player][index]);
	return 4;
}

/// Returns number of available locations
static int scrRecallOilDefendLoc(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	int index = luaL_checkint(L, 2);

	if(index < 0 || index >= MAX_OIL_DEFEND_LOCATIONS)
	{
		return luaL_argerror(L, 2, "wrong index");
	}

	//check if can recall at this location
	if(!CanRememberPlayerOilDefenseLoc(player, index))
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	lua_pushinteger(L, oilDefendLocation[player][index][0]);
	lua_pushinteger(L, oilDefendLocation[player][index][1]);
	lua_pushinteger(L, oilDefendLocPrior[player][index]);
	return 4;
}


/* Restores vilibility (fog of war) */
WZ_DECL_UNUSED static BOOL scrRecallPlayerVisibility(void)
{
	SDWORD				player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrRecallPlayerVisibility(): stack failed");
		return false;
	}

	if(player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR,"scrRecallPlayerVisibility: player index too high.");
		return false;
	}



	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrSavePlayerAIExperience(void)
{
	SDWORD				player;
	BOOL				bNotify;

	if (!stackPopParams(2, VAL_INT, &player, VAL_BOOL, &bNotify))
	{
		debug(LOG_ERROR, "scrSavePlayerAIExperience(): stack failed");
		return false;
	}

	scrFunctionResult.v.bval = SavePlayerAIExperience(player, bNotify);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

static int scrLoadPlayerAIExperience(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	
	lua_pushinteger(L, LoadPlayerAIExperience(player));
	return 1;
}


/* Add a beacon (blip) */
BOOL addBeaconBlip(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * textMsg)
{
	MESSAGE			*psMessage;
	VIEWDATA		*pTempData;

	if (forPlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "addBeaconBlip: player number is too high");
		return false;
	}

	//find the message if was already added previously
	psMessage = findBeaconMsg(forPlayer, sender);
	if (psMessage)
	{
		//remove it
		removeMessage(psMessage, forPlayer);
	}

	//create new message
	psMessage = addBeaconMessage(MSG_PROXIMITY, false, forPlayer);
	if (psMessage)
	{
		pTempData = CreateBeaconViewData(sender, locX, locY);

		ASSERT(pTempData != NULL, "Empty help data for radar beacon");

		psMessage->pViewData = (MSG_VIEWDATA *)pTempData;

		debug(LOG_MSG, "blip added, pViewData=%p", psMessage->pViewData);
	}
	else
	{
		debug(LOG_MSG, "call failed");
	}

	//Received a blip message from a player callback
	//store and call later
	//-------------------------------------------------
	//call beacon callback only if not adding for ourselves
	if(forPlayer != sender)
	{
		if(!msgStackPush(CALL_BEACON,sender,forPlayer,textMsg,locX,locY,NULL))
		{
			debug(LOG_ERROR, "addBeaconBlip() - msgStackPush - stack failed");
			return false;
		}

		if(selectedPlayer == forPlayer)
		{
			// show console message
			CONPRINTF(ConsoleString,(ConsoleString, _("Beacon received from %s!"),
				getPlayerName(sender)));

			// play audio
			audio_QueueTrackPos( ID_SOUND_BEACON, locX, locY, 0);
		}
	}

	return true;
}

BOOL sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * beaconMsg)
{
	if(sender == forPlayer || myResponsibility(forPlayer))	//if destination player is on this machine
	{
		debug(LOG_WZ,"sending beacon to player %d (local player) from %d", forPlayer, sender);
		return addBeaconBlip(locX, locY, forPlayer, sender, beaconMsg);
	}
	else
	{
		debug(LOG_WZ,"sending beacon to player %d (remote player) from %d", forPlayer, sender);
		return sendBeacon(locX, locY, forPlayer, sender, beaconMsg);
	}
}

//prepare viewdata for help blip
VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY)
{
	UDWORD				height;
	VIEWDATA			*psViewData;
	SDWORD				audioID;
	char				name[MAXSTRLEN];

	//allocate message space
	psViewData = (VIEWDATA *)malloc(sizeof(VIEWDATA));
	if (psViewData == NULL)
	{
		ASSERT(false, "prepairHelpViewData() - Unable to allocate memory for viewdata");
		return NULL;
	}

	memset(psViewData, 0, sizeof(VIEWDATA));

	psViewData->numText = 1;

	//store name
	sprintf(name, _("Beacon %d"), sender);
 	psViewData->pName = name;

	//allocate space for text strings
	psViewData->ppTextMsg = (const char **) malloc(sizeof(char *));

	//store text message, hardcoded for now
	psViewData->ppTextMsg[0] = strdup(getPlayerName(sender));

	//store message type
	psViewData->type = VIEW_BEACON;

	//allocate memory for blip location etc
	psViewData->pData = (VIEW_PROXIMITY *) malloc(sizeof(VIEW_PROXIMITY));
	if (psViewData->pData == NULL)
	{
		ASSERT(false, "prepairHelpViewData() - Unable to allocate memory");
		return NULL;
	}

	//store audio
	audioID = NO_SOUND;
	((VIEW_PROXIMITY *)psViewData->pData)->audioID = audioID;

	//store blip location
	((VIEW_PROXIMITY *)psViewData->pData)->x = (UDWORD)LocX;
	((VIEW_PROXIMITY *)psViewData->pData)->y = (UDWORD)LocY;

	//check the z value is at least the height of the terrain
	height = map_Height(LocX, LocY);

	((VIEW_PROXIMITY *)psViewData->pData)->z = height;

	//store prox message type
	((VIEW_PROXIMITY *)psViewData->pData)->proxType = PROX_ENEMY; //PROX_ENEMY for now

	//remember who sent this msg, so we could remove this one, when same player sends a new help-blip msg
	((VIEW_PROXIMITY *)psViewData->pData)->sender = sender;

	//remember when the message was created so can remove it after some time
	((VIEW_PROXIMITY *)psViewData->pData)->timeAdded = gameTime;
	debug(LOG_MSG, "Added message");

	return psViewData;
}

/* Looks through the players list of messages to find VIEW_BEACON (one per player!) pointer */
MESSAGE * findBeaconMsg(UDWORD player, SDWORD sender)
{
	MESSAGE					*psCurr;

	for (psCurr = apsMessages[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		//look for VIEW_BEACON, should only be 1 per player
		if (psCurr->dataType == MSG_DATA_BEACON)
		{
			if(((VIEWDATA *)psCurr->pViewData)->type == VIEW_BEACON)
			{
				debug(LOG_WZ, "findBeaconMsg: %d ALREADY HAS A MESSAGE STORED", player);
				if(((VIEW_PROXIMITY *)((VIEWDATA *)psCurr->pViewData)->pData)->sender == sender)
				{
					debug(LOG_WZ, "findBeaconMsg: %d ALREADY HAS A MESSAGE STORED from %d", player, sender);
					return psCurr;
				}
			}
		}
	}

	//not found the message so return NULL
	return NULL;
}

/* Add beacon (radar blip) */
WZ_DECL_UNUSED static BOOL scrDropBeacon(void)
{
	SDWORD			forPlayer,sender;
	char					ssval2[255];
	UDWORD			locX,locY,locZ;

	if (!stackPopParams(6, VAL_STRING, &strParam1 , VAL_INT, &forPlayer,
				VAL_INT, &sender, VAL_INT, &locX, VAL_INT, &locY, VAL_INT, &locZ))
	{
		debug(LOG_ERROR,"scrDropBeacon failed to pop parameters");
		return false;
	}

	ssprintf(ssval2, "%s : %s", getPlayerName(sender), strParam1);	//temporary solution

	return sendBeaconToPlayer(locX, locY, forPlayer, sender, ssval2);
}

/* Remove beacon from the map */
WZ_DECL_UNUSED static BOOL scrRemoveBeacon(void)
{
	MESSAGE			*psMessage;
	SDWORD			player, sender;

	if (!stackPopParams(2, VAL_INT, &player, VAL_INT, &sender))
	{
		debug(LOG_ERROR,"scrRemoveBeacon: failed to pop parameters");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR,"scrRemoveBeacon:player number is too high");
		return false;
	}

	if (sender >= MAX_PLAYERS)
	{
		debug(LOG_ERROR,"scrRemoveBeacon:sender number is too high");
		return false;
	}

	//find the message
	psMessage = findBeaconMsg(player, sender);
	if (psMessage)
	{
		//delete it
		removeMessage(psMessage, player);
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrClosestDamagedGroupDroid(void)
{
	DROID_GROUP	*psGroup;
	DROID		*psDroid,*psClosestDroid;
	SDWORD		x,y,healthLeft,wBestDist,wDist,maxRepairedBy,player;

	if (!stackPopParams(6, VAL_INT, &player, ST_GROUP, &psGroup, VAL_INT, &healthLeft,
		VAL_INT, &x, VAL_INT, &y, VAL_INT, &maxRepairedBy))
	{
		debug(LOG_ERROR, "scrClosestDamagedGroupDroid: failed to pop");
		return false;
	}

	wBestDist = 999999;
	psClosestDroid = NULL;
	for(psDroid = psGroup->psList;psDroid; psDroid = psDroid->psGrpNext)
	{
		if((psDroid->body * 100 / psDroid->originalBody) <= healthLeft)	//in%
		{
			wDist = map_coord(iHypot(psDroid->pos.x - x, psDroid->pos.y - y));  //in tiles
			if(wDist < wBestDist)
			{
				if((maxRepairedBy < 0) || (getNumRepairedBy(psDroid, player) <= maxRepairedBy))
				{
					psClosestDroid = psDroid;
					wBestDist = wDist;
				}
			}
		}
	}

	scrFunctionResult.v.oval = psClosestDroid;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

SDWORD getNumRepairedBy(DROID *psDroidToCheck, SDWORD player)
{
	DROID		*psDroid;
	SDWORD		numRepaired = 0;

	for(psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if((psDroid->droidType != DROID_REPAIR) && (psDroid->droidType != DROID_CYBORG_REPAIR))
		{
			continue;
		}

		if (psDroid->psTarget != NULL && psDroid->psTarget->type == OBJ_DROID)
		{
			if ((DROID *)psDroid->psTarget == psDroidToCheck)
			{
				numRepaired++;
			}
		}
	}

	return numRepaired;
}

/// Uses debug_console() for console debug output right now
static int scrMsgBox(lua_State *L)
{
	const char *message = luaL_checkstring(L, 1);
	debug_console("DEBUG: %s",message);
	return 0;
}


// Check for a struct being within a certain range of a position (must be visible)
WZ_DECL_UNUSED static BOOL scrStructInRangeVis(void)
{
	SDWORD		range, player,lookingPlayer, x,y;
	BOOL		found;

	if (!stackPopParams(5, VAL_INT, &lookingPlayer, VAL_INT, &player , VAL_INT, &x, VAL_INT, &y, VAL_INT, &range))
	{
		debug(LOG_ERROR, "scrStructInRangeVis: failed to pop");
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT(false, "scrStructInRange: invalid player number");
		return false;
	}

	found = objectInRangeVis((BASE_OBJECT *)apsStructLists[player], x,y, range, lookingPlayer);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Check for a droid being within a certain range of a position (must be visible)
WZ_DECL_UNUSED static BOOL scrDroidInRangeVis(void)
{
	SDWORD		range, player,lookingPlayer, x,y;
	BOOL		found;

	if (!stackPopParams(5, VAL_INT, &lookingPlayer, VAL_INT, &player , VAL_INT, &x, VAL_INT, &y, VAL_INT, &range))
	{
		debug(LOG_ERROR, "scrDroidInRangeVis: failed to pop");
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT(false, "scrDroidInRangeVis: invalid player number");
		return false;
	}

	found = objectInRangeVis((BASE_OBJECT *)apsDroidLists[player], x,y, range, lookingPlayer);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// check for a base object being in range of a point
BOOL objectInRangeVis(BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range, SDWORD lookingPlayer)
{
	BASE_OBJECT		*psCurr;
	SDWORD			xdiff, ydiff, rangeSq;

	// See if there is a droid in range
	rangeSq = range * range;
	for(psCurr = psList; psCurr; psCurr = psCurr->psNext)
	{
		if(psCurr->type == OBJ_STRUCTURE)
		{
			if(!((STRUCTURE *)psCurr)->visible[lookingPlayer])
				continue;
		}

		if(psCurr->type == OBJ_DROID)
		{
			if(!((DROID *)psCurr)->visible[lookingPlayer])
				continue;
		}

		// skip partially build structures
		//if ( (psCurr->type == OBJ_STRUCTURE) &&
		//	 (((STRUCTURE *)psCurr)->status != SS_BUILT) )
		//{
		//	continue;
		//}

		// skip flying vtols
		if ( (psCurr->type == OBJ_DROID) &&
			isVtolDroid((DROID *)psCurr) &&
			((DROID *)psCurr)->sMove.Status != MOVEINACTIVE )
		{
			continue;
		}

		xdiff = (SDWORD)psCurr->pos.x - x;
		ydiff = (SDWORD)psCurr->pos.y - y;
		if (xdiff*xdiff + ydiff*ydiff < rangeSq)
		{
			return true;
		}
	}

	return false;
}

/// Go after a certain research
static int scrPursueResearch(lua_State *L)
{
	SDWORD				foundIndex = 0, cur, tempIndex, Stack[400];
	UDWORD				index;
	SWORD				top;
	BOOL				found;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH_FACILITY	*psResFacilty;

	STRUCTURE *psBuilding = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);
	int player = luaWZ_checkplayer(L, 2);
	const char *researchName = luaL_checkstring(L, 3);
	
	RESEARCH *psResearch = getResearch(researchName, false);

	if (psResearch == NULL)
	{
		return luaL_error(L, "no such research topic: %s", researchName);
	}

	psResFacilty =	(RESEARCH_FACILITY*)psBuilding->pFunctionality;

	if (psResFacilty->psSubject != NULL)		// not finished yet
	{
		scrFunctionResult.v.bval = false;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			return false;
		}
		return true;
	}

	pPlayerRes = asPlayerResList[player];
	index = psResearch - asResearch;

	if (index >= numResearch)
	{
		debug( LOG_ERROR, "invalid research index" );
		return luaL_error(L, "internal error");
	}

	found = false;

	if (beingResearchedByAlly(index, player))		//an ally is already researching it
	{
		found = false;
	}
	else if (IsResearchCompleted(&pPlayerRes[index]))
	{
		found = false;
		//DbgMsg("Research already completed: %d", index);
	}
	else if (IsResearchStarted(&pPlayerRes[index]))
	{
		found = false;
		//DbgMsg("Research already in progress, %d", index);
	}
	else if (IsResearchPossible(&pPlayerRes[index]) || IsResearchCancelled(&pPlayerRes[index]))
	{
		foundIndex = index;
		found = true;
		//DbgMsg("Research possible or cancelled: %d", index);
	}
	else if (skTopicAvail(index, player))
	{
		foundIndex = index;
		found = true;
		//DbgMsg("Research available: %d",index);
	}
	else
	{
		//DbgMsg("starting to search for: %d, %s", index, asResearch[index].pName);
		top = -1;

		cur = 0;				//start with first index's PR
		tempIndex = -1;
		while(true)	//do
		{
			//DbgMsg("Going on with %d, numPR: %d, %s", index, asResearch[index].numPRRequired, asResearch[index].pName);

			if (cur >= asResearch[index].numPRRequired)		//node has nodes?
			{
				//DbgMsg("cur >= numPRRequired : %d (%d >= %d)", index, cur, asResearch[index].numPRRequired);

				top = top - 2;
				if (top < (-1))
				{
					//DbgMsg("Nothing on stack");
					break;		//end of stack
				}
				index = Stack[top + 2];	//if index = -1, then exit
				cur = Stack[top + 1];		//go to next PR of the last node, since this one didn't work

			}
			else		//end of nodes not reached
			{
				tempIndex = asResearch[index].pPRList[cur];		//get cur node's index
				//DbgMsg("evaluating node: %d, (cur = %d), %s", tempIndex, cur, asResearch[tempIndex].pName);

				if (skTopicAvail(tempIndex,player) && (!beingResearchedByAlly(tempIndex, player)))	//<NEW> - ally check added
				{
					//DbgMsg("avail: %d (cur=%d), %s", tempIndex, cur, asResearch[tempIndex].pName);
					found = true;
					foundIndex = tempIndex;		//done
					break;
				}
				else if ((IsResearchCompleted(&pPlayerRes[tempIndex]) == false) && (IsResearchStarted(&pPlayerRes[tempIndex]) == false))		//not avail and not busy with it, can check this PR's PR
				{
					//DbgMsg("node not complete, not started: %d, (cur=%d), %s", tempIndex,cur, asResearch[tempIndex].pName);
					if (asResearch[tempIndex].numPRRequired > 0)	//node has any nodes itself
					{
						//DbgMsg("node has nodes, so selecting as main node: %d, %s", tempIndex, asResearch[tempIndex].pName);

						Stack[top+1] = cur;								//so can go back to it further
						Stack[top+2] = index;
						top = top + 2;

						index = tempIndex;		//go 1 level further
						cur = -1;									//start with first PR of this PR next time
					}
					else		//has no PRs, choose it (?)
					{
						if(!beingResearchedByAlly(tempIndex, player))	//<NEW> ally check added
						{
							//DbgMsg("PR has no PRs, choosing it: %d (cur=%d), %s", tempIndex, cur, asResearch[tempIndex].pName);
							found = true;
							foundIndex = tempIndex;	//done
							break;
						}
					}
				}
			}

			cur++;				//try next node of the main node
			if((cur >= asResearch[index].numPRRequired) && (top <= (-1)))	//nothing left
			{
				break;
			}

		} // while(true)
	}

	if (found && foundIndex < numResearch)
	{
		sendResearchStatus(psBuilding, foundIndex, player, true);	// inform others, I'm researching this.
#if defined (DEBUG)
		{
			char	sTemp[128];
			sprintf(sTemp,"player:%d starts topic: %s",player, asResearch[foundIndex].pName );
			NETlogEntry(sTemp, SYNC_FLAG, 0);
		}
#endif
	}

	lua_pushboolean(L, found);
	return 1;
	intRefreshScreen();
}

WZ_DECL_UNUSED static BOOL scrGetStructureType(void)
{
	STRUCTURE			*psStruct;

	if (!stackPopParams(1, ST_STRUCTURE, &psStruct))
	{
		debug(LOG_ERROR, "scrGetStructureType(): stack failed");
		return false;
	}

	scrFunctionResult.v.ival = psStruct->pStructureType->type;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetStructureType(): failed to push result");
		return false;
	}

	return true;
}

/// Get player name from index
static int scrGetPlayerName(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	
	lua_pushstring(L, getPlayerName(player));
	return 1;
}

/// Set player name
static int scrSetPlayerName(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	const char *name = luaL_checkstring(L, 2);

	lua_pushboolean(L, setPlayerName(player, name));
	return 1;
}

SDWORD getPlayerFromString(char *playerName)
{
	UDWORD	playerIndex;
	char	sPlayerNumber[255];

	for( playerIndex=0; playerIndex<MAX_PLAYERS; playerIndex++ )
	{
		/* check name */
		//debug(LOG_SCRIPT, "checking  (%s,%s)",getPlayerName(playerIndex), playerName);
		if (strncasecmp(getPlayerName(playerIndex), playerName, 255) == 0)
		{
			//debug(LOG_SCRIPT, "matched, returning %d", playerIndex);
			return playerIndex;
		}

		/* check color */
		//debug(LOG_SCRIPT, "checking (%s,%s)",getPlayerColourName(playerIndex), playerName);
		if (strncasecmp(getPlayerColourName(playerIndex), playerName, 255) == 0)
		{
			//debug(LOG_SCRIPT, "matched, returning %d", playerIndex);
			return playerIndex;
		}

		/* check player number */
		sprintf(sPlayerNumber,"%d",playerIndex);
		//debug(LOG_SCRIPT, "checking (%s,%s)",sPlayerNumber, playerName);
		if (strncasecmp(sPlayerNumber,playerName, 255) == 0)
		{
			//debug(LOG_SCRIPT, "matched, returning %d", playerIndex);
			return playerIndex;
		}

	}

	return -1;
}

/* Checks if a particular bit is set in an integer */
WZ_DECL_UNUSED static BOOL scrGetBit(void)
{
	SDWORD				val1,val2;

	if (!stackPopParams(2, VAL_INT, &val1, VAL_INT, &val2))
	{
		debug(LOG_ERROR, "scrGetBit(): failed to pop");
		return false;
	}

	ASSERT(val2 < MAX_PLAYERS && val2 >= 0, "scrGetBit(): wrong player index (%d)", val2);

	scrFunctionResult.v.bval = ((val1 & bitMask[val2]) != 0);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Sets a particular bit in an integer */
WZ_DECL_UNUSED static BOOL scrSetBit(void)
{
	SDWORD				base,position;
	BOOL				bSet;

	if (!stackPopParams(3, VAL_INT, &base,
		VAL_INT, &position, VAL_BOOL, &bSet))
	{
		debug(LOG_ERROR, "scrSetBit(): failed to pop");
		return false;
	}

	ASSERT(position < MAX_PLAYERS && position >= 0, "scrSetBit(): wrong position index (%d)", position);

	if(bSet)
	{
		base |= bitMask[position];
	}
	else
	{
		base &= bitMask[position];
	}

	scrFunctionResult.v.ival = base;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/// Can we create and break alliances?
static int scrAlliancesLocked(lua_State *L)
{
	lua_pushboolean(L, !(bMultiPlayer && (game.alliance == ALLIANCES)));
	return 1;
}

static int scrLockAllicances(lua_State *L)
{
	bool lock = luaL_checkboolean(L, 1);
	if (lock)
	{
		game.alliance = ALLIANCES_TEAMS;
	}
	else
	{
		game.alliance = ALLIANCES;
	}
	return 0;
}

/* Visualize radius at position */
WZ_DECL_UNUSED static BOOL scrShowRangeAtPos(void)
{
	SDWORD		x,y,radius;

	if (!stackPopParams(3, VAL_INT, &x, VAL_INT, &y, VAL_INT, &radius))
	{
		debug(LOG_ERROR, "scrShowRangeAtPos(): stack failed");
		return false;
	}

	//Turn on/off drawing
	showRangeAtPos(x,y,radius);

	return true;
}

/* Exponential function */
WZ_DECL_UNUSED static BOOL scrExp(void)
{
	float		fArg;

	if (!stackPopParams(1, VAL_FLOAT, &fArg))
	{
		return false;
	}

	scrFunctionResult.v.fval = exp(fArg);
	if (!stackPushResult(VAL_FLOAT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Square root */
WZ_DECL_UNUSED static BOOL scrSqrt(void)
{
	float		fArg;

	if (!stackPopParams(1, VAL_FLOAT, &fArg))
	{
		return false;
	}

	scrFunctionResult.v.fval = sqrtf(fArg);
	if (!stackPushResult(VAL_FLOAT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Natural logarithm */
WZ_DECL_UNUSED static BOOL scrLog(void)
{
	float		fArg;

	if (!stackPopParams(1, VAL_FLOAT, &fArg))
	{
		return false;
	}

	scrFunctionResult.v.fval = log(fArg);
	if (!stackPushResult(VAL_FLOAT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Show/Hide multiplayer debug menu */
WZ_DECL_UNUSED static BOOL scrDebugMenu(void)
{
	SDWORD		menuUp;

	if (!stackPopParams(1, VAL_BOOL, &menuUp))
	{
		debug(LOG_ERROR, "scrDebugMenu(): stack failed");
		return false;
	}

	(void)addDebugMenu(menuUp);

	return true;
}

/* Set debug menu output string */
WZ_DECL_UNUSED static BOOL scrSetDebugMenuEntry(void)
{
	SDWORD		index;

	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_INT, &index))
	{
		debug(LOG_ERROR, "scrSetDebugMenuEntry(): stack failed");
		return false;
	}

	setDebugMenuEntry(strParam1, index);

	return true;
}

/// Parse chat message and return number of commands that could be extracted
static int scrProcessChatMsg(lua_State *L)
{
	const char *message = luaL_checkstring(L, 1);
	strlcpy(strParam1, message, MAXSTRLEN);
	
	debug(LOG_NEVER, "Now preparing to parse '%s'", strParam1);

	if (!chatLoad(strParam1, strlen(strParam1)))
	{
		return luaL_error(L, "Couldn't process chat message: %s", strParam1);
	}

	lua_pushinteger(L, chat_msg.numCommands);
	return 1;
}

/* Returns number of command arguments for a certain
 * chat command that could be extracted
 */
WZ_DECL_UNUSED static BOOL scrGetNumArgsInCmd(void)
{
	SDWORD		cmdIndex;

	if (!stackPopParams(1, VAL_INT, &cmdIndex))
	{
		debug(LOG_ERROR, "scrGetNumArgsInCmd(): stack failed");
		return false;
	}

	/* Check command bounds */
	if(cmdIndex < 0 || cmdIndex >= chat_msg.numCommands)
	{
		ASSERT(false, "scrGetNumArgsInCmd: command inxed out of bounds: %d (num commands: %d)",
			cmdIndex, chat_msg.numCommands);
		return false;
	}


	scrFunctionResult.v.ival = chat_msg.cmdData[cmdIndex].numCmdParams;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetNumArgsInCmd(): failed to push result");
		return false;
	}

	return true;
}

/* Returns a string representing a certain chat command,
 * based on the command index provided
 */
WZ_DECL_UNUSED static BOOL scrGetChatCmdDescription(void)
{
	SDWORD			cmdIndex;
	char			*pChatCommand=NULL;

	if (!stackPopParams(1, VAL_INT, &cmdIndex))
	{
		debug(LOG_ERROR, "scrGetCommandDescription(): stack failed");
		return false;
	}

	/* Check command bounds */
	if(cmdIndex < 0 || cmdIndex >= chat_msg.numCommands)
	{
		ASSERT(false, "scrGetCommandDescription: command inxed out of bounds: %d (num commands: %d)",
			cmdIndex, chat_msg.numCommands);
		return false;
	}

	/* Allocate memory for the comamnd string */
	pChatCommand = (char*)malloc(MAXSTRLEN);
	if (pChatCommand == NULL)
	{
		debug(LOG_FATAL, "scrGetCmdDescription: Out of memory!");
		abort();
		return false;
	}

	/* Copy command */
	strlcpy(pChatCommand, chat_msg.cmdData[cmdIndex].pCmdDescription, MAXSTRLEN);

	/* Make scrFunctionResult point to the valid command */
	scrFunctionResult.v.sval = pChatCommand;

	if (!stackPushResult(VAL_STRING, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetCommandDescription(): failed to push result");
		free(pChatCommand);
		return false;
	}

	free(pChatCommand);

	return true;
}

/* Returns a certain parameter of a certain chat command
 * Returns false if failed
 */
WZ_DECL_UNUSED static BOOL scrGetChatCmdParam(void)
{
	SDWORD			cmdIndex, argIndex;
	void			*pArgument=NULL;
	INTERP_TYPE		argType=VAL_VOID;
	BOOL			bSuccess=true;		//failure on type mismatch

	//if (!stackPopParams(3, VAL_INT, &cmdIndex, VAL_INT, &argIndex, VAL_REF | VAL_VOID, &pArgument))
	//{
	//	debug(LOG_ERROR, "scrGetChatCmdParam(): stack failed");
	//	return false;
	//}

	if (!stackPopParams(2, VAL_INT, &cmdIndex, VAL_INT, &argIndex))
	{
		debug(LOG_ERROR, "scrGetChatCmdParam(): stack failed");
		return false;
	}

	if(cmdIndex < 0 || cmdIndex >= chat_msg.numCommands)
	{
		ASSERT(false, "scrGetChatCmdParam: command index out of bounds: %d", cmdIndex);
		return false;
	}

	if(argIndex < 0 || argIndex >= chat_msg.cmdData[cmdIndex].numCmdParams )
	{
		ASSERT(false, "scrGetChatCmdParam: argument index for command %d is out of bounds: %d", cmdIndex, argIndex);
		return false;
	}

	/* Find out the type of the argument we are going to pass to the script */
	argType = chat_msg.cmdData[cmdIndex].parameter[argIndex].type;

	if (!stackPopParams(1, VAL_REF | argType, &pArgument))
	{
		debug(LOG_ERROR, "scrGetChatCmdParam(): stack failed or argument mismatch (expected type of argument: %d)", argType);
		bSuccess = false;		//return type mismatch
		//return false;
	}

	if(pArgument == NULL)
	{
		ASSERT(false, "scrGetChatCmdParam: nullpointer check failed");
		bSuccess = false;
		//return false;
	}

	/* Return command argument to the script */
	if(bSuccess){
		memcpy(pArgument, &(chat_msg.cmdData[cmdIndex].parameter[argIndex].v), sizeof(chat_msg.cmdData[cmdIndex].parameter[argIndex].v));
	}

	scrFunctionResult.v.bval = bSuccess;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetChatCmdParam(): failed to push result");
		return false;
	}
	return true;
}

/* Returns true if a certain command was addressed to a certain player */
WZ_DECL_UNUSED static BOOL scrChatCmdIsPlayerAddressed(void)
{
	SDWORD		cmdIndex,playerInQuestion;

	if (!stackPopParams(2, VAL_INT, &cmdIndex, VAL_INT, &playerInQuestion))
	{
		debug(LOG_ERROR, "scrChatCmdIsPlayerAddressed(): stack failed");
		return false;
	}

	/* Check command bounds */
	if(cmdIndex < 0 || cmdIndex >= chat_msg.numCommands)
	{
		ASSERT(false, "scrChatCmdIsPlayerAddressed: command inxed out of bounds: %d (num commands: %d)",
			cmdIndex, chat_msg.numCommands);
		return false;
	}

	/* Check player bounds */
	if(playerInQuestion < 0 || playerInQuestion >= MAX_PLAYERS)
	{
		ASSERT(false, "scrChatCmdIsPlayerAddressed: player inxed out of bounds: %d", playerInQuestion);
		return false;
	}

	scrFunctionResult.v.bval = chat_msg.cmdData[cmdIndex].bPlayerAddressed[playerInQuestion];
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrChatCmdIsPlayerAddressed(): failed to push result");
		return false;
	}

	return true;
}

/* Modifies height of a tile */
WZ_DECL_UNUSED static BOOL scrSetTileHeight(void)
{
	UDWORD		tileX,tileY,newHeight;
	MAPTILE		*psTile;

	if (!stackPopParams(3, VAL_INT, &tileX, VAL_INT, &tileY, VAL_INT, &newHeight))
	{
		debug(LOG_ERROR, "scrSetTileHeight(): stack failed");
		return false;
	}

	ASSERT(newHeight <= 255, "scrSetTileHeight: height out of bounds");

	psTile = mapTile(tileX,tileY);

	psTile->height = (UBYTE)newHeight * ELEVATION_SCALE;

	return true;
}

/* Returns structure which placed on provided coordinates.
 * Returns NULL (NULLOBJECT) if there's no structure.
 */
static int scrGetTileStructure(lua_State *L)
{
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);
	STRUCTURE *s = getTileStructure(x, y);

	if (s)
	{
		luaWZObj_pushstructure(L, s);
		return 1; 
	}

	// nothing found
	return 0;
}

/* Outputs script call stack
 */
WZ_DECL_UNUSED static BOOL scrPrintCallStack(void)
{
	scrOutputCallTrace(LOG_SCRIPT);

	return true;
}

/*
 * Returns true if game debug mode is on
 */
static int scrDebugModeEnabled(lua_State *L)
{
	lua_pushboolean(L, getDebugMappingStatus());
	return 1;
}

/*
 * Returns the cost of a droid
 */
WZ_DECL_UNUSED static BOOL scrCalcDroidPower(void)
{
	DROID	*psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		return false;
	}

	ASSERT(psDroid != NULL,
		"scrCalcDroidPower: can't calculate cost of a null-droid");

	scrFunctionResult.v.ival = (SDWORD)calcDroidPower(psDroid);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCalcDroidPower(): failed to push result");
		return false;
	}

	return true;
}

/*
 * Returns experience level of a droid
 */
WZ_DECL_UNUSED static BOOL scrGetDroidLevel(void)
{
	DROID	*psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		return false;
	}

	ASSERT(psDroid != NULL,
		"scrGetDroidLevel: null-pointer passed");

	scrFunctionResult.v.ival = (SDWORD)getDroidLevel(psDroid);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetDroidLevel(): failed to push result");
		return false;
	}

	return true;
}

static int scrGetGameTime(lua_State *L)
{
	lua_pushnumber(L, (float)gameTime/1000.0f);
	return 1;
}

static int scrGetStructureByID(lua_State *L)
{
	int id = luaL_checkint(L, 1);
	STRUCTURE *structure;
	
	structure = (STRUCTURE*)getBaseObjFromId(id);
	if (structure == NULL || structure->type != OBJ_STRUCTURE)
	{
		//return luaL_error(L, "invalid structure ID");
		// return nil for now
		lua_pushnil(L);
		return 1;
	}
	
	
	luaWZObj_pushstructure(L, structure);
	return 1;	
}

static int scrDestroyed(lua_State *L)
{
	unsigned int id = luaWZObj_toid(L, 1);
	BASE_OBJECT *object;

	if (id == 0)
	{
		return luaL_error(L, "argument 1 should be an id or an object");
	}
	object = getBaseObjFromId(id);
	if (!object)
	{
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushboolean(L, object->died);
	}
	return 1;
}

/// Assembles a template from components and returns it
static int scrAssembleWeaponTemplate(lua_State *L)
{
	SDWORD					bodyIndex,weapIndex,propIndex;
	DROID_TEMPLATE			*pNewTemplate = NULL;
	
	int player = luaWZ_checkplayer(L, 1);
	const char *bodyName = luaWZObj_checkname(L, 2);
	const char *propulsionName = luaWZObj_checkname(L, 3);
	const char *weaponName = luaWZObj_checkname(L, 4);
	
	debug(LOG_SCRIPT, "assembling template %s + %s + %s", bodyName, propulsionName, weaponName);
	
	bodyIndex = getCompFromName(COMP_BODY, bodyName);
	weapIndex = getCompFromName(COMP_WEAPON, weaponName);
	propIndex = getCompFromName(COMP_PROPULSION, propulsionName);
	
	pNewTemplate  = malloc(sizeof(DROID_TEMPLATE));
	if (pNewTemplate == NULL)
	{
		return luaL_error(L, "Out of memory");
	}

	memset(pNewTemplate, 0, sizeof(DROID_TEMPLATE));

	// set template body
	pNewTemplate->asParts[COMP_BODY] = bodyIndex;

	// set template propulsion
	pNewTemplate->asParts[COMP_PROPULSION] = propIndex;

	// set template weapon (only one)
	pNewTemplate->asWeaps[0] = weapIndex;
	pNewTemplate->numWeaps = 1;

	// set default components
	pNewTemplate->asParts[COMP_SENSOR]		= 0;
	pNewTemplate->asParts[COMP_ECM]			= 0;
	pNewTemplate->asParts[COMP_CONSTRUCT]	= 0;
	pNewTemplate->asParts[COMP_REPAIRUNIT]	= 0;
	pNewTemplate->asParts[COMP_BRAIN]		= 0;

	// set droid type
	pNewTemplate->droidType = DROID_WEAPON;

	// finalize template and set its name
	if(!intValidTemplate(pNewTemplate, GetDefaultTemplateName(pNewTemplate)))
	{
		return luaL_error(L, "could not create template");
	}

	// make sure we have a valid weapon
	if(!checkValidWeaponForProp(pNewTemplate))
	{
		return luaL_error(L, "no valid weapon");
	}
	else
	{
		DROID_TEMPLATE *tempTemplate = NULL;

		// check if an identical template already exists for this player
		tempTemplate = scrCheckTemplateExists(player, pNewTemplate);
		if(tempTemplate == NULL)
		{
			// set template id
			pNewTemplate->multiPlayerID = generateNewObjectId();

			// add template to player template list
			pNewTemplate->psNext = apsDroidTemplates[player];
			apsDroidTemplates[player] = pNewTemplate;		//apsTemplateList?

			if (bMultiMessages)
			{
				sendTemplate(pNewTemplate);
			}
		}
		else
		{
			// free resources
			free(pNewTemplate);

			// already exists, so return it
			pNewTemplate = tempTemplate;
		}
	}
	debug(LOG_SCRIPT, "created new template with name \"%s\" (id %i)", pNewTemplate->aName, pNewTemplate->multiPlayerID);
	luaWZObj_pushtemplate(L, pNewTemplate);
	return 1;
}

/* Checks if template already exists, returns it if yes */
static DROID_TEMPLATE* scrCheckTemplateExists(SDWORD player, DROID_TEMPLATE *psTempl)
{
	DROID_TEMPLATE* psCurrent;
	BOOL equal;

	for (psCurrent = apsDroidTemplates[player]; psCurrent != NULL; psCurrent = psCurrent->psNext)
	{
		unsigned int componentType;
		unsigned int weaponSlot;
		
		equal = true;

		// compare components
		for (componentType = 0; componentType < ARRAY_SIZE(psTempl->asParts); ++componentType)
		{
			if (psTempl->asParts[componentType] != psCurrent->asParts[componentType])
			{
				equal = false;
				break;
			}
		}

		// compare weapon count
		if (psTempl->numWeaps != psCurrent->numWeaps)
		{
			equal = false;
		}

		// compare all weapons separately
		for(weaponSlot = 0; equal && weaponSlot < psTempl->numWeaps; ++weaponSlot)
		{
			if (psTempl->asWeaps[weaponSlot] != psCurrent->asWeaps[weaponSlot])
			{
				equal = false;
				break;
			}
		}
		if (equal)
		{
			// they are equal, so return the current template
			return psCurrent;
		}
	}

	return NULL;
}

typedef enum {
	SHORT_HIT,
	LONG_HIT,
	DAMAGE,
	FIRE_PAUSE
} UPGRADE_TYPE;

static int weaponXUpgrade(lua_State *L, UPGRADE_TYPE type)
{
	const WEAPON_STATS *psWeapStats;
	int result = 0;
	
	int player = luaWZ_checkplayer(L, 1);
	const char *name = luaWZObj_checkname(L, 2);
	int weapIndex = getCompFromName(COMP_WEAPON, name);

	psWeapStats = &asWeaponStats[weapIndex];

	switch (type)
	{
	case SHORT_HIT:
		result = asWeaponUpgrade[player][psWeapStats->weaponSubClass].shortHit;
		break;
	case LONG_HIT:
		result = asWeaponUpgrade[player][psWeapStats->weaponSubClass].longHit;
		break;
	case DAMAGE:
		result = asWeaponUpgrade[player][psWeapStats->weaponSubClass].damage;
		break;
	case FIRE_PAUSE:
		result = asWeaponUpgrade[player][psWeapStats->weaponSubClass].firePause;
		break;
	}
	lua_pushinteger(L, result);
	return 1;
}

static int scrWeaponShortHitUpgrade(lua_State *L)
{
	return weaponXUpgrade(L, SHORT_HIT);
}
static int scrWeaponLongHitUpgrade(lua_State *L)
{
	return weaponXUpgrade(L, LONG_HIT);
}
static int scrWeaponDamageUpgrade(lua_State *L)
{
	return weaponXUpgrade(L, DAMAGE);
}
static int scrWeaponFirePauseUpgrade(lua_State *L)
{
	return weaponXUpgrade(L, FIRE_PAUSE);
}

static int scrIsComponentAvailable(lua_State *L)
{
	int index, type;
	
	int player = luaWZ_checkplayer(L, 1);
	const char *name = luaWZObj_checkname(L, 2);
	
	findComponentByName(name, &type, &index);
	if (type < 0)
	{
		return luaL_error(L, "unknown component: \"%s\"", name);
	}
	lua_pushboolean(L, apCompLists[player][type][index] == AVAILABLE);
	return 1;
}

static int scrGetBodySize(lua_State *L)
{
	const char *name = luaWZObj_checkname(L, 1);
	int bodyIndex = getCompFromName(COMP_BODY, name);

	lua_pushinteger(L, asBodyStats[bodyIndex].size);
	return 1;
}

WZ_DECL_UNUSED static BOOL scrGettext(void)
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		return false;
	}

	scrFunctionResult.v.sval = (char*)gettext(strParam1);

	return stackPushResult((INTERP_TYPE)ST_TEXTSTRING, &scrFunctionResult);
}

WZ_DECL_UNUSED static BOOL scrGettext_noop(void)
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		return false;
	}

	scrFunctionResult.v.sval = gettext_noop(strParam1);

	return stackPushResult(VAL_STRING, &scrFunctionResult);
}

WZ_DECL_UNUSED static BOOL scrPgettext(void)
{
	char* msg_ctxt_id;
	char* translation;

	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_STRING, &strParam2))
	{
		return false;
	}

	if (asprintf(&msg_ctxt_id, "%s%s%s", strParam1, GETTEXT_CONTEXT_GLUE, strParam2) == -1)
	{
		debug(LOG_FATAL, "Out of memory");
		abort();
		return false;
	}

#ifdef DEFAULT_TEXT_DOMAIN
	translation = (char*)dcgettext(DEFAULT_TEXT_DOMAIN, msg_ctxt_id, LC_MESSAGES);
#else
	translation = (char*)dcgettext(NULL,                msg_ctxt_id, LC_MESSAGES);
#endif

	/* Due to the way dcgettext works a pointer comparison is enough, hence
	 * the reason why we free() now.
	 */
	free(msg_ctxt_id);

	if (translation == msg_ctxt_id)
	{
		scrFunctionResult.v.sval = strParam2;
	}
	else
	{
		scrFunctionResult.v.sval = translation;
	}

	return stackPushResult((INTERP_TYPE)ST_TEXTSTRING, &scrFunctionResult);
}

WZ_DECL_UNUSED static BOOL scrPgettext_expr(void)
{
	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_STRING, &strParam2))
	{
		return false;
	}

	scrFunctionResult.v.sval = (char*)pgettext_expr(strParam1, strParam2);

	return stackPushResult((INTERP_TYPE)ST_TEXTSTRING, &scrFunctionResult);
}

WZ_DECL_UNUSED static BOOL scrPgettext_noop(void)
{
	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_STRING, &strParam2))
	{
		return false;
	}

	scrFunctionResult.v.sval = gettext_noop(strParam1);

	return stackPushResult(VAL_STRING, &scrFunctionResult);
}

static int scrGetWeapon(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	int index = getCompFromName(COMP_WEAPON, name);
	luaWZObj_pushweaponstat(L, index);
	return 1;
}

static int scrStructureOnLocation(lua_State *L)
{
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);

	lua_pushboolean(L, TileHasStructure(mapTile(map_coord(x), map_coord(y))));
	return 1;
}

static int scrFireOnLocation(lua_State *L)
{
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);

	lua_pushboolean(L, fireOnLocation(x, y));
	return 1;
}

static int scrDroidHasTemplate(lua_State *L)
{
	DROID *droid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	DROID_TEMPLATE* template = luaWZObj_checktemplate(L, 2);
	lua_pushboolean(L, strcmp(droid->aName, template->aName) == 0);
	return 1;
}

static int scrRevealEntireMap(lua_State *L)
{
	godMode = true; // view all structures and droids
	setRevealStatus(false); // view the entire map
	return 0;
}

/// Register all script functions with the Lua interpreter
void registerScriptfuncs(lua_State *L)
{
	lua_register(L, "centreViewPos",               scrCentreViewPos); 
	lua_register(L, "setRadarZoom",                scrSetRadarZoom);
	lua_register(L, "setPowerLevel",               scrSetPowerLevel); 
	lua_register(L, "enableStructure",             scrEnableStructure); 
	lua_register(L, "setNoGoArea",                 scrSetNoGoArea);
	lua_register(L, "getGameTime",                 scrGetGameTime);
	lua_register(L, "seenStructInArea",            scrSeenStructInArea);
	lua_register(L, "droidInArea",                 scrDroidInArea);
	lua_register(L, "droidInRange",                scrDroidInRange);
	lua_register(L, "numStructsButNotWallsInArea", scrNumStructsButNotWallsInArea);
	lua_register(L, "objectInRange", scrObjectInRange);
	lua_register(L, "structInRange", scrStructInRange);
	lua_register(L, "playerPower", scrPlayerPower);
	lua_register(L, "structInArea", scrStructInArea);
	lua_register(L, "structButNoWallsInArea", scrStructButNoWallsInArea);
	lua_register(L, "numObjectsInArea", scrNumObjectsInArea);
	lua_register(L, "numDroidsInArea", scrNumDroidsInArea);
	lua_register(L, "numStructsInArea", scrNumStructsInArea);
	lua_register(L, "numStructsByTypeInArea", scrNumStructsByTypeInArea);
	lua_register(L, "objectInArea", scrObjectInArea);
	lua_register(L, "isStructureAvailable", scrIsStructureAvailable);
	lua_register(L, "addReticuleButton", scrAddReticuleButton);
	lua_register(L, "removeReticuleButton", scrRemoveReticuleButton);
	lua_register(L, "addMessage", scrAddMessage);
	lua_register(L, "removeMessage", scrRemoveMessage);
	lua_register(L, "takeOverDroidsInArea", scrTakeOverDroidsInArea);
	lua_register(L, "completeResearch", scrCompleteResearch);
	lua_register(L, "addFeature", scrAddFeature);
	lua_register(L, "destroyFeature", scrDestroyFeature);
	lua_register(L, "enableResearch", scrEnableResearch);
	lua_register(L, "anyStructButWallsLeft", scrAnyStructButWallsLeft);
	lua_register(L, "makeComponentAvailable", scrMakeComponentAvailable);
	lua_register(L, "killStructsInArea", scrKillStructsInArea);
	lua_register(L, "setReinforcementTime", scrSetReinforcementTime);
	lua_register(L, "setMissionTime", scrSetMissionTime);
	lua_register(L, "setStructureLimits", scrSetStructureLimits);
	lua_register(L, "anyDroidsLeft", scrAnyDroidsLeft);
	lua_register(L, "gameOverMessage", scrGameOverMessage);
	lua_register(L, "structureBeingBuilt", scrStructureBeingBuilt);
	lua_register(L, "getStructureByID", scrGetStructureByID);
	lua_register(L, "createAlliance", scrCreateAlliance);
	lua_register(L, "setAssemblyPoint", scrSetAssemblyPoint);
	lua_register(L, "structureIdle", scrStructureIdle);
	lua_register(L, "destroyed", scrDestroyed);
	lua_register(L, "startMission", scrStartMission);
	lua_register(L, "droidHasSeen", scrDroidHasSeen);
	lua_register(L, "addConsoleText", scrAddConsoleText);
	lua_register(L, "tagConsoleText", scrTagConsoleText);
	lua_register(L, "playSound", scrPlaySound);
	lua_register(L, "playSoundPos", scrPlaySoundPos);
	lua_register(L, "buildDroid", scrBuildDroid);
	lua_register(L, "setGroupRetreatPoint", scrSetGroupRetreatPoint);
	lua_register(L, "setGroupRetreatForce", scrSetGroupRetreatForce);
	lua_register(L, "setGroupRetreatLeadership", scrSetGroupRetreatLeadership);
	lua_register(L, "addDroid", scrAddDroid);
	lua_register(L, "getStructure", scrGetStructure);
	lua_register(L, "myResponsibility", scrMyResponsibility);
	lua_register(L, "getDroidCount", scrGetDroidCount);
	lua_register(L, "allianceExistsBetween", scrAllianceExistsBetween);
	lua_register(L, "structureBuiltInRange", scrStructureBuiltInRange);
	lua_register(L, "pickStructLocation", scrPickStructLocation);
	lua_register(L, "pickDroidStructLocation", scrPickStructLocationC);
	lua_register(L, "pickStructLocationB", scrPickStructLocationB);
	lua_register(L, "dbgMsgOn", scrDbgMsgOn);
	lua_register(L, "dbg", scrDbg);
	lua_register(L, "isVtol", scrIsVtol);
	lua_register(L, "setCampaignNumber", scrSetCampaignNumber);
	lua_register(L, "setBackgroundFog", scrSetBackgroundFog);
	lua_register(L, "setDepthFog", scrSetDepthFog);
	lua_register(L, "setFogColour", scrSetFogColour);
	lua_register(L, "playIngameCDAudio", scrPlayIngameCDAudio);
	lua_register(L, "numPlayerWeapStructsInRange", scrNumPlayerWeapStructsInRange);
	lua_register(L, "numPlayerWeapDroidsInRange", scrNumPlayerWeapDroidsInRange);
	lua_register(L, "applyLimitSet", scrApplyLimitSet);
	lua_register(L, "numTemplatesInProduction", scrNumTemplatesInProduction);
	lua_register(L, "structureComplete", scrStructureComplete);
	lua_register(L, "isHumanPlayer", scrIsHumanPlayer);
	lua_register(L, "researchFinished", scrResearchFinished);
	lua_register(L, "researchStarted", scrResearchStarted);
	lua_register(L, "pursueResearch", scrPursueResearch);
	lua_register(L, "getNumStructures", scrGetNumStructures);
	lua_register(L, "recallPlayerBaseLoc", scrRecallPlayerBaseLoc);
	lua_register(L, "playerLoaded", scrPlayerLoaded);
	lua_register(L, "getPlayerName", scrGetPlayerName);
	lua_register(L, "hasGroup", scrHasGroup);
	lua_register(L, "getStructureVis", scrGetStructureVis);
	lua_register(L, "getClosestEnemyStructByType", scrGetClosestEnemyStructByType);
	lua_register(L, "mapTileVisible", scrMapTileVisible);
	lua_register(L, "getWeapon", scrGetWeapon);
	lua_register(L, "getPlayerColourName", scrGetPlayerColourName);
	lua_register(L, "getBaseDefendLocCount", scrGetBaseDefendLocCount);
	lua_register(L, "getStructureLimit", scrGetStructureLimit);
	lua_register(L, "setPlayerName", scrSetPlayerName);
	lua_register(L, "mapRevealedInRange", scrMapRevealedInRange);
	lua_register(L, "structureLimitReached", scrStructureLimitReached);
	lua_register(L, "structureBuilt", scrStructureBuilt);
	lua_register(L, "alliancesLocked", scrAlliancesLocked);
	lua_register(L, "numDroidsByComponent", scrNumDroidsByComponent);
	lua_register(L, "loadPlayerAIExperience", scrLoadPlayerAIExperience);
	lua_register(L, "debugFile", scrDebugFile);
	lua_register(L, "processChatMsg", scrProcessChatMsg);
	lua_register(L, "recallBaseDefendLoc", scrRecallBaseDefendLoc);
	lua_register(L, "msgBox", scrMsgBox);
	lua_register(L, "isComponentAvailable", scrIsComponentAvailable);
	lua_register(L, "getBodySize", scrGetBodySize);
	lua_register(L, "weaponShortHitUpgrade", scrWeaponShortHitUpgrade);
	lua_register(L, "weaponLongHitUpgrade", scrWeaponLongHitUpgrade);
	lua_register(L, "weaponDamageUpgrade", scrWeaponDamageUpgrade);
	lua_register(L, "weaponFirePauseUpgrade", scrWeaponFirePauseUpgrade);
	lua_register(L, "threatInRange", scrThreatInRange);
	lua_register(L, "getOilDefendLocCount", scrGetOilDefendLocCount);
	lua_register(L, "getOilDefendLocIndex", scrGetOilDefendLocIndex);
	lua_register(L, "getBaseDefendLocIndex", scrGetBaseDefendLocIndex);
	lua_register(L, "recallOilDefendLoc", scrRecallOilDefendLoc);
	lua_register(L, "assembleWeaponTemplate", scrAssembleWeaponTemplate);
	lua_register(L, "testStructureModule", scrTestStructureModule);
	lua_register(L, "fogTileInRange", scrFogTileInRange);
	lua_register(L, "factoryGetTemplate", scrFactoryGetTemplate);
	lua_register(L, "chooseValidLoc", scrChooseValidLoc);
	lua_register(L, "_getDroidVisibleBy", scrInternalGetDroidVisibleBy);
	lua_register(L, "_getStructureVisibleBy", scrInternalGetStructureVisibleBy);
	lua_register(L, "anyFactoriesLeft", scrAnyFactoriesLeft);
	lua_register(L, "_getFeatureVisibleBy", scrInternalGetFeatureVisibleBy);
	lua_register(L, "structureOnLocation", scrStructureOnLocation);
	lua_register(L, "fireOnLocation", scrFireOnLocation);
	lua_register(L, "playerInAlliance", scrPlayerInAlliance);
	lua_register(L, "circlePerimPoint", scrCirclePerimPoint);
	lua_register(L, "console", scrConsole);
	lua_register(L, "removeDroid", scrRemoveDroid);
	lua_register(L, "addPower", scrAddPower);
	lua_register(L, "droidHasTemplate", scrDroidHasTemplate);
	lua_register(L, "destroyStructure", scrDestroyStructure);
	lua_register(L, "clearConsole", scrClearConsole);
	lua_register(L, "lockAllicances", scrLockAllicances);
	lua_register(L, "revealEntireMap", scrRevealEntireMap);
	lua_register(L, "msg", scrMsg);
	lua_register(L, "getTileStructure", scrGetTileStructure);
	lua_register(L, "setRetreatPoint", scrSetRetreatPoint);
	lua_register(L, "setRetreatForce", scrSetRetreatForce);
	lua_register(L, "setRetreatLeadership", scrSetRetreatLeadership);
	lua_register(L, "setTransporterExit", scrSetTransporterExit);
	lua_register(L, "flyTransporterIn", scrFlyTransporterIn);
	lua_register(L, "flashOn", scrFlashOn);
	lua_register(L, "flashOff", scrFlashOff);
	lua_register(L, "addDroidToTransporter", scrAddDroidToTransporter);
	lua_register(L, "numStructsByStatInRange", scrNumStructsByStatInRange);
	lua_register(L, "debugModeEnabled", scrDebugModeEnabled);
	lua_register(L, "showConsoleText", scrShowConsoleText);
	lua_register(L, "flushConsoleMessages", scrClearConsole);	// FIXME duplicate function
	lua_register(L, "randomiseSeed", scrRandomiseSeed);
	lua_register(L, "initAllNoGoAreas", scrInitAllNoGoAreas);
	lua_register(L, "addDroidToMissionList", scrAddDroidToMissionList);
	lua_register(L, "getPlayer", scrGetPlayer);
	lua_register(L, "getPlayerStartPosition", scrGetPlayerStartPosition);
	lua_register(L, "scavengersActive", scrScavengersActive);
	lua_register(L, "safeDest", scrSafeDest);
	lua_register(L, "threatAt", scrThreatAt);
	//lua_register(L, "", );
	//lua_register(L, "", );
	//lua_register(L, "", );
	//lua_register(L, "", );
}
