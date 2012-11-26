/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#include "lib/framework/wzapp.h"
#include "lib/framework/strres.h"
#include "lib/framework/stdio_ext.h"
#include "lib/widget/widget.h"

#include <time.h>
#include <string.h>

#include "effects.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "lib/gamelib/gtime.h"
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
#include "multiint.h"
#include "advvis.h"
#include "mapgrid.h"
#include "lib/ivis_opengl/piestate.h"
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
#include "display3d.h"			//for showRangeAtPos()
#include "multimenu.h"
#include "lib/script/chat_processing.h"
#include "keymap.h"
#include "visibility.h"
#include "design.h"
#include "random.h"
#include "template.h"

static INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

// If this is defined then check max number of units not reached before adding more.
#define SCRIPT_CHECK_MAX_UNITS

static SDWORD	bitMask[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
static char		strParam1[MAXSTRLEN], strParam2[MAXSTRLEN];		//these should be used as string parameters for stackPopParams()

static bool	structHasModule(STRUCTURE *psStruct);

static DROID_TEMPLATE* scrCheckTemplateExists(SDWORD player, DROID_TEMPLATE *psTempl);

/// Hold the previously assigned player
Vector2i positions[MAX_PLAYERS];
std::vector<Vector2i> derricks;

bool scriptOperatorEquals(INTERP_VAL const &v1, INTERP_VAL const &v2)
{
	ASSERT_OR_RETURN(false, scriptTypeIsPointer(v1.type) && scriptTypeIsPointer(v2.type), "Bad types.");
	if (v1.type == (INTERP_TYPE)ST_RESEARCH && v1.type == (INTERP_TYPE)ST_RESEARCH)
	{
		return ((RESEARCH *)v1.v.oval)->ref == ((RESEARCH *)v2.v.oval)->ref;
	}
	return v1.v.oval == v2.v.oval;
}

void scriptSetStartPos(int position, int x, int y)
{
	positions[position].x = x;
	positions[position].y = y;
	debug(LOG_SCRIPT, "Setting start position %d to (%d, %d)", position, x, y);
}

void scriptSetDerrickPos(int x, int y)
{
	Vector2i pos(x, y);
	derricks.push_back(pos);
}

bool scriptInit()
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		scriptSetStartPos(i, 0, 0);
	}
	derricks.clear();
	derricks.reserve(8 * MAX_PLAYERS);
	return true;
}

bool scrScavengersActive()
{
	scrFunctionResult.v.ival = scavengerPlayer();
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

bool scrGetDifficulty()
{
	int player;
	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Bad player index");
	scrFunctionResult.v.ival = NetPlay.players[player].difficulty;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		ASSERT(false, "Failed to initialize player");
		return false;
	}
	return true;
}

bool scrGetPlayer()
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		debug(LOG_ERROR, "stack failed");
		return false;
	}
	int i = scrFunctionResult.v.ival = getNextAIAssignment(strParam1);
	debug(LOG_SCRIPT, "Initialized player %d, starts at position (%d, %d), max %d players", i, positions[i].x, positions[i].y, game.maxPlayers);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		ASSERT(false, "Failed to initialize player");
		return false;
	}
	return true;
}

bool scrGetDerrick()
{
	int x, y, i;

	if (!stackPopParams(1, VAL_INT, &i))
	{
		debug(LOG_ERROR, "stack failed");
		return false;
	}
	scrFunctionResult.v.oval = NULL;
	if (i < (int)derricks.size())
	{
		x = derricks[i].x;
		y = derricks[i].y;
		MAPTILE *psTile = worldTile(x, y);
		ASSERT(psTile, "No derrick or oil at position!");
		if (psTile)
		{
			scrFunctionResult.v.oval = psTile->psObject;
		}
	}
	if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))
	{
		ASSERT(false, "Failed to push result");
		return false;
	}
	return true;
}

Vector2i getPlayerStartPosition(int player)
{
	return positions[player];
}

bool scrSetSunPosition(void)
{
	float x, y, z;

	if (!stackPopParams(3, VAL_FLOAT, &x, VAL_FLOAT, &y, VAL_FLOAT, &z))
	{
		return false;
	}
	setTheSun(Vector3f(x, y, z));
	return true;
}

bool scrSetSunIntensity(void)
{
	float ambient[4], diffuse[4], specular[4];

	// Scary list of parameters... ambient, diffuse and specular RGB components
	// One day we should add support for vectors to our scripting language to cut
	// down on such noise.
	if (!stackPopParams(9, VAL_FLOAT, &ambient[0], VAL_FLOAT, &ambient[1], VAL_FLOAT, &ambient[2],
	                       VAL_FLOAT, &diffuse[0], VAL_FLOAT, &diffuse[1], VAL_FLOAT, &diffuse[2],
	                       VAL_FLOAT, &specular[0], VAL_FLOAT, &specular[1], VAL_FLOAT, &specular[2]))
	{
		return false;
	}
	ambient[3] = 1.0;
	diffuse[3] = 1.0;
	specular[3] = 1.0;
	pie_Lighting0(LIGHT_AMBIENT, ambient);
	pie_Lighting0(LIGHT_DIFFUSE, diffuse);
	pie_Lighting0(LIGHT_SPECULAR, specular);
	return true;
}

bool scrSafeDest(void)
{
	SDWORD	x, y, player;

	if (!stackPopParams(3, VAL_INT, &player, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, player < game.maxPlayers, "Out of bounds player index");
	ASSERT_OR_RETURN(false, worldOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	scrFunctionResult.v.bval = !(auxTile(map_coord(x), map_coord(y), player) & AUXBITS_DANGER);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

bool scrThreatAt(void)
{
	SDWORD	x, y, player;

	if (!stackPopParams(3, VAL_INT, &player, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, player < game.maxPlayers, "Out of bounds player index");
	ASSERT_OR_RETURN(false, worldOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	scrFunctionResult.v.bval = auxTile(map_coord(x), map_coord(y), player) & AUXBITS_THREAT;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

bool scrGetPlayerStartPosition(void)
{
	SDWORD	*x, *y, player;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|VAL_INT, &x, VAL_REF|VAL_INT, &y))
	{
		return false;
	}
	if (player < game.maxPlayers)
	{
		*x = positions[player].x;
		*y = positions[player].y;
		scrFunctionResult.v.bval = true;
	}
	else
	{
		*x = 0;
		*y = 0;
		scrFunctionResult.v.bval = false;
	}

	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

/******************************************************************************************/
/*                 Check for objects in areas                                             */


// check for a base object being in range of a point
bool objectInRange(BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range)
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
bool scrObjectInRange(void)
{
	SDWORD		range, player, x,y;
	bool		found;

	if (!stackPopParams(4, VAL_INT, &player, VAL_INT, &x, VAL_INT, &y, VAL_INT, &range))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrObjectInRange: invalid player number" );
		return false;
	}

	found = objectInRange((BASE_OBJECT *)apsDroidLists[player], x,y, range) ||
			objectInRange((BASE_OBJECT *)apsStructLists[player], x,y, range);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Check for a droid being within a certain range of a position
bool scrDroidInRange(void)
{
	SDWORD		range, player, x,y;
	bool		found;

	if (!stackPopParams(4, VAL_INT, &player, VAL_INT, &x, VAL_INT, &y, VAL_INT, &range))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrUnitInRange: invalid player number" );
		return false;
	}

	found = objectInRange((BASE_OBJECT *)apsDroidLists[player], x,y, range);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Check for a struct being within a certain range of a position
bool scrStructInRange(void)
{
	SDWORD		range, player, x,y;
	bool		found;

	if (!stackPopParams(4, VAL_INT, &player, VAL_INT, &x, VAL_INT, &y, VAL_INT, &range))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrStructInRange: invalid player number" );
		return false;
	}

	found = objectInRange((BASE_OBJECT *)apsStructLists[player], x,y, range);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
bool scrPlayerPower(void)
{
	SDWORD player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}
	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrPlayerPower: invalid player number" );
		return false;
	}

	scrFunctionResult.v.ival = getPower(player);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}
	return true;
}


// -----------------------------------------------------------------------------------------
// check for a base object being in an area
static bool objectInArea(BASE_OBJECT *psList, SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
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
bool scrObjectInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	bool		found;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrObjectInArea: invalid player number" );
		return false;
	}

	found = objectInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2) ||
			objectInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Check for a droid being within a certain area
bool scrDroidInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	bool		found;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrUnitInArea: invalid player number" );
		return false;
	}

	found = objectInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Check for a struct being within a certain Area of a position
bool scrStructInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	bool		found;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrStructInArea: invalid player number" );
		return false;
	}

	found = objectInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
bool scrSeenStructInArea(void)
{
	int32_t	walls=false;	// was BOOL (int) ** see warning about conversion
	bool	found = false;
	SDWORD		player,enemy,x1,y1, x2,y2;
	STRUCTURE	*psCurr;
	SDWORD		ox,oy;

	// player, enemyplayer, walls, x1,r1,x2,y2
	if (!stackPopParams(7, VAL_INT, &player, VAL_INT, &enemy, VAL_BOOL,&walls,VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}


	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSeenStructInArea: invalid player number" );
		return false;
	}

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

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Check for a players structures but no walls being within a certain area
bool scrStructButNoWallsInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	SDWORD		ox,oy;
	STRUCTURE	*psStruct;
	SDWORD		found = false;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrStructButNoWallsInArea: invalid player number" );
		return false;
	}

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

	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
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
bool scrNumObjectsInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	SDWORD		count;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumObjectsInArea: invalid player number" );
		return false;
	}

	count = numObjectsInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2) +
			numObjectsInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	scrFunctionResult.v.ival = count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// Count the number of player droids within a certain area
bool scrNumDroidsInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	SDWORD		count;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumUnitInArea: invalid player number" );
		return false;
	}

	count = numObjectsInArea((BASE_OBJECT *)apsDroidLists[player], x1,y1, x2,y2);

	scrFunctionResult.v.ival = count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// Count the number of player structures within a certain area
bool scrNumStructsInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	SDWORD		count;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumStructsInArea: invalid player number" );
		return false;
	}

	count = numObjectsInArea((BASE_OBJECT *)apsStructLists[player], x1,y1, x2,y2);

	scrFunctionResult.v.ival = count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// Count the number of player structures but not walls within a certain area
bool scrNumStructsButNotWallsInArea(void)
{
	SDWORD		player, x1,y1, x2,y2;
	SDWORD		count, ox,oy;
	STRUCTURE	*psStruct;

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumStructsButNotWallsInArea: invalid player number" );
		return false;
	}

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

	scrFunctionResult.v.ival = count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// Count the number of structures in an area of a certain type
bool scrNumStructsByTypeInArea(void)
{
	SDWORD		player, type, x1,y1, x2,y2;
	SDWORD		count, ox,oy;
	STRUCTURE	*psStruct;

	if (!stackPopParams(6, VAL_INT, &player, VAL_INT, &type,
					VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumStructsByTypeInArea: invalid player number" );
		return false;
	}

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

	scrFunctionResult.v.ival = count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// Check for a droid having seen a certain object
bool scrDroidHasSeen(void)
{
	SDWORD		player;
	BASE_OBJECT	*psObj;
	bool		seen;

	if (!stackPopParams(2, ST_BASEOBJECT, &psObj, VAL_INT, &player))
	{
		return false;
	}

	if (psObj == NULL)
	{
		ASSERT( false, "scrUnitHasSeen: NULL object" );
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrUnitHasSeen:player number is too high" );
		return false;
	}

	// See if any droid has seen this object
	seen = false;
	if (psObj->visible[player])
	{
		seen = true;
	}

	scrFunctionResult.v.bval = seen;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Enable a component to be researched
bool scrEnableComponent(void)
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

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrEnableComponent:player number is too high" );
		return false;
	}

	// enable the appropriate component
	switch ((unsigned)sVal.type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
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
// Make a component available
bool scrMakeComponentAvailable(void)
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

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrMakeComponentAvailable:player number is too high" );
		return false;
	}

	// make the appropriate component available
	switch ((unsigned)sVal.type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
	{
	case ST_BODY:
		apCompLists[player][COMP_BODY][sVal.v.ival] = AVAILABLE;
		break;
	case ST_PROPULSION:
		apCompLists[player][COMP_PROPULSION][sVal.v.ival] = AVAILABLE;
		break;
	case ST_ECM:
		apCompLists[player][COMP_ECM][sVal.v.ival] = AVAILABLE;
		break;
	case ST_SENSOR:
		apCompLists[player][COMP_SENSOR][sVal.v.ival] = AVAILABLE;
		break;
	case ST_CONSTRUCT:
		apCompLists[player][COMP_CONSTRUCT][sVal.v.ival] = AVAILABLE;
		break;
	case ST_WEAPON:
		apCompLists[player][COMP_WEAPON][sVal.v.ival] = AVAILABLE;
		break;
	case ST_REPAIR:
		apCompLists[player][COMP_REPAIRUNIT][sVal.v.ival] = AVAILABLE;
		break;
	case ST_BRAIN:
		apCompLists[player][COMP_BRAIN][sVal.v.ival] = AVAILABLE;
		break;
	default:
		ASSERT( false, "scrEnableComponent: unknown type" );
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Add a droid
bool scrAddDroidToMissionList(void)
{
	SDWORD			player;
	DROID_TEMPLATE	*psTemplate;
	DROID			*psDroid;

	if (!stackPopParams(2, ST_TEMPLATE, &psTemplate, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddUnitToMissionList:player number is too high" );
		return false;
	}

	ASSERT( psTemplate != NULL,
		"scrAddUnitToMissionList: Invalid template pointer" );

#ifdef SCRIPT_CHECK_MAX_UNITS
	// Don't build a new droid if player limit reached, unless it's a transporter.
	if( IsPlayerDroidLimitReached(player) && (psTemplate->droidType != DROID_TRANSPORTER && psTemplate->droidType != DROID_SUPERTRANSPORTER))
	{
		debug( LOG_NEVER, "scrAddUnit : Max units reached ,player %d\n", player );
		psDroid = NULL;
	} else
#endif
	{
		psDroid = buildMissionDroid( psTemplate, 128, 128, player );
	}

	scrFunctionResult.v.oval = psDroid;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Add a droid
bool scrAddDroid(void)
{
	SDWORD			x, y, player;
	DROID_TEMPLATE	*psTemplate;
	DROID			*psDroid;

	if (!stackPopParams(4, ST_TEMPLATE, &psTemplate, VAL_INT, &x, VAL_INT, &y, VAL_INT, &player))
	{
		return false;
	}
	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddUnit:player number is too high" );
		return false;
	}

	ASSERT( psTemplate != NULL,
		"scrAddUnit: Invalid template pointer" );

#ifdef SCRIPT_CHECK_MAX_UNITS
	// Don't build a new droid if player limit reached, unless it's a transporter.
	if( IsPlayerDroidLimitReached(player) && (psTemplate->droidType != DROID_TRANSPORTER && psTemplate->droidType != DROID_SUPERTRANSPORTER) )
	{
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

	scrFunctionResult.v.oval = psDroid;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Add droid to transporter

bool scrAddDroidToTransporter(void)
{
	DROID	*psTransporter, *psDroid;

	if (!stackPopParams(2, ST_DROID, &psTransporter, ST_DROID, &psDroid))
	{
		return false;
	}

	if (psTransporter == NULL || psDroid == NULL)
	{
		ASSERT(false, "scrAddUnitToTransporter: null unit passed");
		return true; // allow to continue
	}

	ASSERT(psTransporter != NULL, "scrAddUnitToTransporter: invalid transporter pointer");
	ASSERT(psDroid != NULL, "scrAddUnitToTransporter: invalid unit pointer");
	ASSERT((psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER), "scrAddUnitToTransporter: invalid transporter type");

	/* check for space */
	if (checkTransporterSpace(psTransporter, psDroid))
	{
		if (droidRemove(psDroid, mission.apsDroidLists))
		{
			psTransporter->psGroup->add(psDroid);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//check for a building to have been destroyed
bool scrBuildingDestroyed(void)
{
	SDWORD		player;
	UDWORD		structureID;
	bool		destroyed;
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
bool scrEnableStructure(void)
{
	SDWORD		player, index;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &index, VAL_INT, &player))
	{
		return false;
	}
	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrEnableStructure:player number is too high" );
		return false;
	}

	if (index < (SDWORD)0 || index > (SDWORD)numStructureStats)
	{
		ASSERT( false, "scrEnableStructure:invalid structure stat" );
		return false;
	}

	// enable the appropriate structure
	apStructTypeLists[player][index] = AVAILABLE;

	return true;
}



// -----------------------------------------------------------------------------------------
// Check if a structure can be built.
// currently PC skirmish only.
bool scrIsStructureAvailable(void)
{
	SDWORD		player, index;
	bool		bResult;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &index, VAL_INT, &player))
	{
		return false;
	}
	if (apStructTypeLists[player][index] == AVAILABLE
	    && asStructLimits[player][index].currentQuantity < asStructLimits[player][index].limit)
	{
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	scrFunctionResult.v.bval = bResult;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}


// -----------------------------------------------------------------------------------------
//make the droid with the matching id the currently selected droid
bool scrSelectDroidByID(void)
{
	SDWORD			player, droidID;
	bool			selected;

	if (!stackPopParams(2, ST_DROIDID, &droidID, VAL_INT, &player))
	{
		return false;
	}
	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSelectUnitByID:player number is too high" );
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
bool scrNumMB(void)
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
bool scrApproxRoot(void)
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
bool scrAddReticuleButton(void)
{
	SDWORD	val;

	if (!stackPopParams(1, VAL_INT, &val))
	{
		return false;
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
		ASSERT( false, "scrAddReticuleButton: Invalid reticule Button ID" );
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//Remove a reticule button from the interface
bool scrRemoveReticuleButton(void)
{
	SDWORD	val;
	int32_t	bReset;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(2, VAL_INT, &val,VAL_BOOL, &bReset))
	{
		return false;
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
		ASSERT( false, "scrAddReticuleButton: Invalid reticule Button ID" );
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// add a message to the Intelligence Display
bool scrAddMessage(void)
{
	MESSAGE			*psMessage;
	MESSAGE_TYPE		msgType;
	SDWORD			player;
	int32_t			playImmediate; // was BOOL (int) ** see warning about conversion
	VIEWDATA		*psViewData;
	UDWORD			height;


	if (!stackPopParams(4, ST_INTMESSAGE, &psViewData , VAL_INT, &msgType,
				VAL_INT, &player, VAL_BOOL, &playImmediate))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddMessage:player number is too high" );
		return false;
	}

	//create the message
	psMessage = addMessage(msgType, false, player);
	if (psMessage)
	{
		//set the data
		psMessage->pViewData = (MSG_VIEWDATA *)psViewData;
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

		if (playImmediate)
		{
			displayImmediateMessage(psMessage);
			// FIXME: We should add some kind of check to see if the FMVs are available or not, and enable this based on that.
			// If we want to inform user of a message, we shouldn't stop the flash right?
			//stopReticuleButtonFlash(IDRET_INTEL_MAP);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// remove a message from the Intelligence Display
bool scrRemoveMessage(void)
{
	MESSAGE			*psMessage;
	MESSAGE_TYPE		msgType;
	SDWORD			player;
	VIEWDATA		*psViewData;

	if (!stackPopParams(3, ST_INTMESSAGE, &psViewData , VAL_INT, &msgType, VAL_INT, &player))
	{
		return false;
	}

	ASSERT(player < MAX_PLAYERS && player >= 0, "bad player number");
	if (player >= MAX_PLAYERS || player < 0)
	{
		return false;
	}

	//find the message
	psMessage = findMessage((MSG_VIEWDATA *)psViewData, msgType, player);
	if (psMessage)
	{
		//delete it
		debug(LOG_MSG, "Removing %s", psViewData->pName);
		removeMessage(psMessage, player);
	}
	else
	{
		debug(LOG_ERROR, "cannot find message - %s", psViewData->pName);
	}

	return true;
}

// -----------------------------------------------------------------------------------------
/*builds a droid in the specified factory*/
bool scrBuildDroid(void)
{
	SDWORD			player, productionRun;
	STRUCTURE		*psFactory;
	DROID_TEMPLATE	*psTemplate;

	if (!stackPopParams(4, ST_TEMPLATE, &psTemplate, ST_STRUCTURE, &psFactory, VAL_INT, &player, VAL_INT, &productionRun))
	{
		return false;
	}

	ASSERT_OR_RETURN(false, psFactory != NULL, "NULL factory object");
	ASSERT_OR_RETURN(false, psTemplate != NULL, "NULL template object sent to %s", objInfo((BASE_OBJECT *)psFactory));
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player number");
	ASSERT_OR_RETURN(false, productionRun <= UBYTE_MAX, "Production run too high");
	ASSERT_OR_RETURN(false, (psFactory->pStructureType->type == REF_FACTORY ||
	                         psFactory->pStructureType->type == REF_CYBORG_FACTORY ||
	                         psFactory->pStructureType->type == REF_VTOL_FACTORY),
	                 "Structure is not a factory");
	ASSERT_OR_RETURN(false, validTemplateForFactory(psTemplate, psFactory, true), "Invalid template - %s for factory - %s",
	                 psTemplate->aName, psFactory->pStructureType->pName);
	if (productionRun != 1)
	{
		debug(LOG_WARNING, "A wzscript is trying to build a different number (%d) than 1 droid.", productionRun);
	}
	if (researchedTemplate(psTemplate, psFactory->player, true, true))
	{
		structSetManufacture(psFactory, psTemplate, ModeQueue);
	}
	else
	{
		debug(LOG_WARNING, "A wzscript tried to build a template (%s) that has not been researched yet", psTemplate->aName);
	}
	return true;
}

// -----------------------------------------------------------------------------------------
// for a specified structure, set the assembly point droids go to when built
bool	scrSetAssemblyPoint(void)
{
	SDWORD		x, y;
	STRUCTURE	*psBuilding;

	if (!stackPopParams(3, ST_STRUCTURE, &psBuilding, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}

	if (psBuilding == NULL)
	{
		ASSERT( false, "scrSetAssemblyPoint: NULL structure" );
		return false;
	}

	if (psBuilding->pStructureType->type != REF_FACTORY &&
		psBuilding->pStructureType->type != REF_CYBORG_FACTORY &&
		psBuilding->pStructureType->type != REF_VTOL_FACTORY)
	{
		ASSERT( false, "scrSetAssemblyPoint: structure is not a factory" );
		return false;
	}

	setAssemblyPoint(((FACTORY *)psBuilding->pFunctionality)->psAssemblyPoint,x,y,
        psBuilding->player, true);

	return true;
}

// -----------------------------------------------------------------------------------------
// test for structure is idle or not
bool	scrStructureIdle(void)
{
	STRUCTURE	*psBuilding;
	bool		idle;

	if (!stackPopParams(1, ST_STRUCTURE, &psBuilding))
	{
		return false;
	}
	if (psBuilding == NULL)
	{
		ASSERT( false, "scrStructureIdle: NULL structure" );
		return false;
	}

	idle = structureIdle(psBuilding);
	scrFunctionResult.v.bval = idle;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// sends a players droids to a location to attack
bool	scrAttackLocation(void)
{
	SDWORD		player, x, y;

	if (!stackPopParams(3, VAL_INT, &x, VAL_INT, &y, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAttackLocation:player number is too high" );
		return false;
	}

	debug(LOG_ERROR, "UNUSED FUNCTION attackLocation called - do not use!");

	return true;
}

// -----------------------------------------------------------------------------------------
//Destroy a feature
bool scrDestroyFeature(void)
{
	FEATURE		*psFeature;
//	INTERP_VAL	sVal;

	if (!stackPopParams(1, ST_FEATURE, &psFeature))
	{
		return false;
	}

	if (psFeature == NULL)
	{
		ASSERT( psFeature != NULL,
			"scrDestroyFeature: Invalid feature pointer" );
	}

	removeFeature(psFeature);

	return true;
}

// -----------------------------------------------------------------------------------------
// static vars to enum features.
static	FEATURE_STATS	*psFeatureStatToFind[MAX_PLAYERS];
static	SDWORD			playerToEnum[MAX_PLAYERS];
static  SDWORD			getFeatureCount[MAX_PLAYERS]={0};
static	FEATURE			*psCurrEnumFeature[MAX_PLAYERS];

// -----------------------------------------------------------------------------------------
// Init enum visible features. May use player==-1 to ignore visibility check.
bool scrInitGetFeature(void)
{
	SDWORD			player,iFeat,bucket;

	if (!stackPopParams(3, ST_FEATURESTAT, &iFeat, VAL_INT, &player, VAL_INT, &bucket))
	{
		return false;
	}

	ASSERT_OR_RETURN(false, bucket >= 0 && bucket < MAX_PLAYERS, "Bucket out of bounds: %d", bucket);
	ASSERT_OR_RETURN(false, (player >= 0 || player == -1) && player < MAX_PLAYERS, "Player out of bounds: %d", player);

	psFeatureStatToFind[bucket]		= (FEATURE_STATS *)(asFeatureStats + iFeat);				// find this stat
	playerToEnum[bucket]			= player;				// that this player can see
	psCurrEnumFeature[bucket]		= apsFeatureLists[0];
	getFeatureCount[bucket]			= 0;					// start at the beginning of list.
	return true;
}

// -----------------------------------------------------------------------------------------
// get next visible feature of required type
// notes:	can't rely on just using the feature list, since it may change
//			between calls, Use an index into list instead.
//			Doesn't return Features sharing a tile with a structure.
//			Skirmish Only, dunno if kev uses this?
bool scrGetFeature(void)
{
	SDWORD	bucket,count;
	FEATURE	*psFeat;

	if ( !stackPopParams(1,VAL_INT,&bucket) )
	{
		ASSERT( false, "scrGetFeature: Failed to pop player number from stack" );
		return false;
	}

	ASSERT(bucket >= 0 && bucket < MAX_PLAYERS,
		"scrGetFeature: bucket out of bounds: %d", bucket);

	count =0;
	// go to the correct start point in the feature list.
	for(psFeat=apsFeatureLists[0];psFeat && count<getFeatureCount[bucket] ;count++)
	{
		psFeat = psFeat->psNext;
	}

	if(psFeat == NULL)		// no more to find.
	{
		scrFunctionResult.v.oval = NULL;
		if (!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))
		{
			ASSERT( false, "scrGetFeature: Failed to push result" );
			return false;
		}
		return true;
	}

	// check to see if badly called
	if(psFeatureStatToFind[bucket] == NULL)
	{
		debug( LOG_NEVER, "invalid feature to find. possibly due to save game\n" );
		scrFunctionResult.v.oval = NULL;
		if(!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))
		{
			ASSERT( false, "scrGetFeature: Failed to push result" );
			return false;
		}
		return true;
	}

	// begin searching the feature list for the required stat.
	while(psFeat)
	{
		if (psFeat->psStats->subType == psFeatureStatToFind[bucket]->subType
		 && (playerToEnum[bucket] < 0 || psFeat->visible[playerToEnum[bucket]] != 0)
		 && !TileHasStructure(mapTile(map_coord(psFeat->pos.x), map_coord(psFeat->pos.y)))
		 && !fireOnLocation(psFeat->pos.x,psFeat->pos.y)		// not burning.
		   )
		{
			scrFunctionResult.v.oval = psFeat;
			if (!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))	//	push scrFunctionResult
			{
				ASSERT( false, "scrGetFeature: Failed to push result" );
				return false;
			}

			getFeatureCount[bucket]++;
			return true;
		}
		getFeatureCount[bucket]++;
		psFeat=psFeat->psNext;
	}

	// none found
	scrFunctionResult.v.oval = NULL;
	if (!stackPushResult((INTERP_TYPE)ST_FEATURE,  &scrFunctionResult))
	{
		ASSERT( false, "scrGetFeature: Failed to push result" );
		return false;
	}
	return true;
}

/* Faster implementation of scrGetFeature -  assumes no features are deleted between calls */
bool scrGetFeatureB(void)
{
	SDWORD	bucket;

	if ( !stackPopParams(1,VAL_INT,&bucket) )
	{
		ASSERT( false, "scrGetFeatureB: Failed to pop player number from stack" );
		return false;
	}

	ASSERT(bucket >= 0 && bucket < MAX_PLAYERS,
		"scrGetFeatureB: bucket out of bounds: %d", bucket);

	// check to see if badly called
	if(psFeatureStatToFind[bucket] == NULL)
	{
		debug( LOG_NEVER, "invalid feature to find. possibly due to save game\n" );
		scrFunctionResult.v.oval = NULL;
		if(!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))
		{
			ASSERT( false, "scrGetFeatureB: Failed to push result" );
			return false;
		}
		return true;
	}

	// begin searching the feature list for the required stat.
	while(psCurrEnumFeature[bucket])
	{
		if (psCurrEnumFeature[bucket]->psStats->subType == psFeatureStatToFind[bucket]->subType
		    && (playerToEnum[bucket] < 0 || psCurrEnumFeature[bucket]->visible[playerToEnum[bucket]] != 0)
		    && !TileHasStructure(mapTile(map_coord(psCurrEnumFeature[bucket]->pos.x), map_coord(psCurrEnumFeature[bucket]->pos.y)))
		    && !fireOnLocation(psCurrEnumFeature[bucket]->pos.x,psCurrEnumFeature[bucket]->pos.y )		// not burning.
			)
		{
			scrFunctionResult.v.oval = psCurrEnumFeature[bucket];
			if (!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))	//	push scrFunctionResult
			{
				ASSERT( false, "scrGetFeatureB: Failed to push result" );
				return false;
			}
			psCurrEnumFeature[bucket] = psCurrEnumFeature[bucket]->psNext;
			return true;
		}

		psCurrEnumFeature[bucket] = psCurrEnumFeature[bucket]->psNext;
	}

	// none found
	scrFunctionResult.v.oval = NULL;
	if (!stackPushResult((INTERP_TYPE)ST_FEATURE,  &scrFunctionResult))
	{
		ASSERT( false, "scrGetFeatureB: Failed to push result" );
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
//Add a feature
bool scrAddFeature(void)
{
	FEATURE_STATS	*psStat;
	FEATURE			*psFeat = NULL;
	SDWORD			iX, iY, iMapX, iMapY, iTestX, iTestY, iFeat;

	if ( !stackPopParams(3, ST_FEATURESTAT, &iFeat,
		 VAL_INT, &iX, VAL_INT, &iY ) )
	{
		return false;
	}

	psStat = (FEATURE_STATS *)(asFeatureStats + iFeat);

	ASSERT(psStat != NULL, "Invalid feature pointer");

	if ( psStat != NULL )
	{
		iMapX = map_coord(iX);
		iMapY = map_coord(iY);

		/* check for wrecked feature already on-tile and remove */
		for(psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
		{
			iTestX = map_coord(psFeat->pos.x);
			iTestY = map_coord(psFeat->pos.y);

			ASSERT(iTestX != iMapX || iTestY != iMapY, "Building feature on tile already occupied");
		}

		psFeat = buildFeature( psStat, iX, iY, false );
	}

	scrFunctionResult.v.oval = psFeat;
	if (!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//Add a structure
bool scrAddStructure(void)
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
bool scrDestroyStructure(void)
{
	STRUCTURE	*psStruct;

	if (!stackPopParams(1, ST_STRUCTURE, &psStruct))
	{
		return false;
	}

	if (psStruct == NULL)
	{
		ASSERT( psStruct != NULL,
			"scrDestroyStructure: Invalid structure pointer" );
	}

	removeStruct( psStruct, true );

	return true;
}



// -----------------------------------------------------------------------------------------
//NEXT 2 FUNCS ONLY USED IN MULTIPLAYER AS FAR AS I KNOW (25 AUG98) alexl.
// static vars to enum structs;
static	STRUCTURE_STATS	*psStructStatToFind;
static	UDWORD			playerToEnumStruct;
static	UDWORD			enumStructCount;
static	bool			structfindany;
static	SDWORD			playerVisibleStruct;		//player whose structures must be visible

//for the bucket version
static	STRUCTURE_STATS	*psStructStatToFindB[MAX_PLAYERS];
static	UDWORD			playerToEnumStructB[MAX_PLAYERS];
static	UDWORD			enumStructCountB[MAX_PLAYERS];
static	int32_t			structfindanyB[MAX_PLAYERS];			// was BOOL (int) ** see warning about conversion
static	SDWORD			playerVisibleStructB[MAX_PLAYERS];		//player whose structures must be visible
// init enum visible structures.
bool scrInitEnumStruct(void)
{
	SDWORD		lookingPlayer,iStat,targetPlayer;
	int32_t		any; // was BOOL (int) ** see warning about conversion

	if ( !stackPopParams(4,VAL_BOOL,&any, ST_STRUCTURESTAT, &iStat,  VAL_INT, &targetPlayer, VAL_INT, &lookingPlayer) )
	{
		return false;
	}

	ASSERT(targetPlayer >= 0 && targetPlayer < MAX_PLAYERS,
		"scrInitEnumStructB: targetPlayer out of bounds: %d", targetPlayer);

	ASSERT(lookingPlayer >= 0 && lookingPlayer < MAX_PLAYERS,
		"scrInitEnumStructB: lookingPlayer out of bounds: %d", lookingPlayer);

	structfindany = any;

	psStructStatToFind	= (STRUCTURE_STATS *)(asStructureStats + iStat);
	playerToEnumStruct	= (UDWORD)targetPlayer;
	playerVisibleStruct = lookingPlayer;		//remember who must be able to see the structure
	enumStructCount		= 0;
	return true;
}

// -----------------------------------------------------------------------------------------
bool scrEnumStruct(void)
{
	UDWORD		count;
	STRUCTURE	*psStruct;

	// go to the correct start point in the structure list.
	count = 0;
	for(psStruct=apsStructLists[playerToEnumStruct];psStruct && count<enumStructCount;count++)
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
//		if(	(structfindany || (psStruct->pStructureType->type == psStructStatToFind->type))
		if(	(structfindany || (psStruct->pStructureType->ref == psStructStatToFind->ref))
			&&
			((playerVisibleStruct < 0) || (psStruct->visible[playerVisibleStruct]))	//fix: added playerVisibleStruct for visibility test
			)
		{
			scrFunctionResult.v.oval = psStruct;
			if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))			//	push scrFunctionResult
			{
				return false;
			}
			enumStructCount++;

			return true;
		}
		enumStructCount++;
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

// init enum visible structures - takes bucket as additional parameter
bool scrInitEnumStructB(void)
{
	SDWORD		lookingPlayer,iStat,targetPlayer,bucket;
	int32_t		any; // was BOOL (int) ** see warning about conversion

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
bool scrEnumStructB(void)
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
/*looks to see if a structure (specified by type) exists and is being built*/
bool scrStructureBeingBuilt(void)
{
//	INTERP_VAL			sVal;
	UDWORD				structInc;
	STRUCTURE_STATS		*psStats;
	SDWORD				player;
	bool				beingBuilt;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &structInc, VAL_INT, &player))
	{
		return false;
	}

/*	if (!stackPop(&sVal))
	{
		return false;
	}

	if (sVal.type != ST_STRUCTURESTAT)
	{
		ASSERT( false, "scrStructureBeingBuilt: type mismatch for object" );
		return false;
	}
	psStats = (STRUCTURE_STATS *)(asStructureStats + sVal.v.ival);
*/
	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrStructureBeingBuilt:player number is too high" );
		return false;
	}

	ASSERT_OR_RETURN( false, structInc < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", structInc, numStructureStats);
	psStats = (STRUCTURE_STATS *)(asStructureStats + structInc);
	beingBuilt = false;
	if (checkStructureStatus(psStats, player, SS_BEING_BUILT))
	{
		beingBuilt = true;
	}

	scrFunctionResult.v.bval = beingBuilt;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}



// -----------------------------------------------------------------------------------------
// multiplayer skirmish only for now.
// returns true if a specific struct is complete. I know it's like the previous func,
bool scrStructureComplete(void)
{
	STRUCTURE	*psStruct;
	bool		bResult;

	if (!stackPopParams(1, ST_STRUCTURE, &psStruct))
	{
		return false;
	}
	if(psStruct->status == SS_BUILT)
	{
		bResult = true;
	}
	else
	{
		bResult = false;
	}

	scrFunctionResult.v.bval = bResult;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}



// -----------------------------------------------------------------------------------------
/*looks to see if a structure (specified by type) exists and built*/
bool scrStructureBuilt(void)
{
	UDWORD				structInc;
	STRUCTURE_STATS		*psStats;
	SDWORD				player;
	bool				built;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &structInc, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrStructureBuilt:player number is too high" );
		return false;
	}

	ASSERT_OR_RETURN( false, structInc < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", structInc, numStructureStats);
	psStats = (STRUCTURE_STATS *)(asStructureStats + structInc);

	built = false;
	if (checkStructureStatus(psStats, player, SS_BUILT))
	{
		built = true;
	}

	scrFunctionResult.v.bval = built;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
/*centre the view on an object - can be droid/structure or feature */
bool scrCentreView(void)
{
	BASE_OBJECT	*psObj;

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
bool scrCentreViewPos(void)
{
	SDWORD		x,y;

	if (!stackPopParams(2, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}

	if ( (x < 0) || (x >= (SDWORD)mapWidth*TILE_UNITS) ||
		 (y < 0) || (y >= (SDWORD)mapHeight*TILE_UNITS))
	{
		ASSERT( false, "scrCenterViewPos: coords off map" );
		return false;
	}

	//centre the view on the objects x/y
	setViewPos(map_coord(x), map_coord(y), false);

	return true;
}

static STRUCTURE *unbuiltIter = NULL;
static int unbuiltPlayer = -1;

bool scrEnumUnbuilt(void)
{
	if (!stackPopParams(1, VAL_INT, &unbuiltPlayer))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, unbuiltPlayer < MAX_PLAYERS && unbuiltPlayer >= 0, "Player number %d out of bounds", unbuiltPlayer);
	unbuiltIter = apsStructLists[unbuiltPlayer];
	return true;
}

bool scrIterateUnbuilt(void)
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
// Get a pointer to a structure based on a stat - returns NULL if cannot find one
bool scrGetStructure(void)
{
	SDWORD				player, index;
	STRUCTURE			*psStruct;
	UDWORD				structType;
	bool				found;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &index, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGetStructure:player number is too high" );
		return false;
	}

	structType = asStructureStats[index].ref;

	//search the players' list of built structures to see if one exists
	found = false;
	for (psStruct = apsStructLists[player]; psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->pStructureType->ref == structType)
		{
			found = true;
			break;
		}
	}

	//make sure pass NULL back if not got one
	if (!found)
	{
		psStruct = NULL;
	}

	scrFunctionResult.v.oval = psStruct;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Get a pointer to a template based on a component stat - returns NULL if cannot find one
bool scrGetTemplate(void)
{
	SDWORD				player;
	DROID_TEMPLATE		*psTemplate;
	bool				found;
	INTERP_VAL			sVal;
	UDWORD				i;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGetTemplate:player number is too high" );
		return false;
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
		switch ((unsigned)sVal.type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
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
bool scrGetDroid(void)
{
	SDWORD				player;
	DROID				*psDroid;
	bool				found;
	INTERP_VAL			sVal;
	UDWORD				i;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGetUnit:player number is too high" );
		return false;
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
		switch ((unsigned)sVal.type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
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
bool scrSetScrollParams(void)
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
bool scrSetScrollMinX(void)
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
bool scrSetScrollMinY(void)
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
bool scrSetScrollMaxX(void)
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
bool scrSetScrollMaxY(void)
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
bool scrSetDefaultSensor(void)
{
	SDWORD				player;
	UDWORD				sensorInc;

	if (!stackPopParams(2, ST_SENSOR, &sensorInc, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
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
bool scrSetDefaultECM(void)
{
	SDWORD				player;
	UDWORD				ecmInc;

	if (!stackPopParams(2, ST_ECM, &ecmInc, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
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
bool scrSetDefaultRepair(void)
{
	SDWORD				player;
	UDWORD				repairInc;

	if (!stackPopParams(2, ST_REPAIR, &repairInc, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
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
// Sets the structure limits for a player
bool scrSetStructureLimits(void)
{
	SDWORD				player, limit;
	UDWORD				structInc;
	STRUCTURE_LIMITS	*psStructLimits;

	if (!stackPopParams(3, ST_STRUCTURESTAT, &structInc, VAL_INT, &limit, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetStructureLimits:player number is too high" );
		return false;
	}

	if (structInc > numStructureStats)
	{
		ASSERT( false, "scrSetStructureLimits: Structure stat is too high - %d", structInc );
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

	psStructLimits = asStructLimits[player];
	psStructLimits[structInc].limit = limit;

	psStructLimits[structInc].globalLimit = limit;

	return true;
}



// -----------------------------------------------------------------------------------------
// multiplayer limit handler.
bool scrApplyLimitSet(void)
{
	applyLimitSet();
	return true;
}



// -----------------------------------------------------------------------------------------
// plays a sound for the specified player - only plays the sound if the
// specified player = selectedPlayer
bool scrPlaySound(void)
{
	SDWORD	player, soundID;

	if (!stackPopParams(2, ST_SOUND, &soundID, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrPlaySound:player number is too high" );
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
	return true;
}

// -----------------------------------------------------------------------------------------
// plays a sound for the specified player - only plays the sound if the
// specified player = selectedPlayer - saves position
bool scrPlaySoundPos(void)
{
	SDWORD	player, soundID, iX, iY, iZ;

	if (!stackPopParams(5, ST_SOUND, &soundID, VAL_INT, &player,
							VAL_INT, &iX, VAL_INT, &iY, VAL_INT, &iZ))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrPlaySoundPos:player number is too high" );
		return false;
	}

	if (player == (SDWORD)selectedPlayer)
	{
		audio_QueueTrackPos(soundID, iX, iY, iZ);
	}
	return true;
}

// -----------------------------------------------------------------------------------------

/* add a text message to the top of the screen for the selected player*/
bool scrShowConsoleText(void)
{
	char				*pText;
	SDWORD				player;

	if (!stackPopParams(2, ST_TEXTSTRING, &pText, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddConsoleText:player number is too high" );
		return false;
	}

	if (player == (SDWORD)selectedPlayer)
	{
		permitNewConsoleMessages(true);
		addConsoleMessage(pText, CENTRE_JUSTIFY, SYSTEM_MESSAGE);
	}

	return true;
}

// -----------------------------------------------------------------------------------------
/* add a text message to the top of the screen for the selected player*/
bool scrAddConsoleText(void)
{
	char				*pText;
	SDWORD				player;

	if (!stackPopParams(2, ST_TEXTSTRING, &pText, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddConsoleText:player number is too high" );
		return false;
	}

	if (player == (SDWORD)selectedPlayer)
	{
		permitNewConsoleMessages(true);
		setConsolePermanence(true,true);
		addConsoleMessage(pText, CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		permitNewConsoleMessages(false);
	}

	return true;
}



// -----------------------------------------------------------------------------------------
/* add a text message to the top of the screen for the selected player - without clearing whats there*/
bool scrTagConsoleText(void)
{
	char				*pText;
	SDWORD				player;

	if (!stackPopParams(2, ST_TEXTSTRING, &pText, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddConsoleText:player number is too high" );
		return false;
	}

	if (player == (SDWORD)selectedPlayer)
	{
		permitNewConsoleMessages(true);
		setConsolePermanence(true,false);
		addConsoleMessage(pText, CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		permitNewConsoleMessages(false);
	}

	return true;
}


// -----------------------------------------------------------------------------------------

bool	scrClearConsole(void)
{
	flushConsoleMessages();
	return(true);
}

// -----------------------------------------------------------------------------------------
//demo functions for turning the power on
bool scrTurnPowerOff(void)
{
	//powerCalculated = false;
	powerCalc(false);

	return true;
}

// -----------------------------------------------------------------------------------------
//demo functions for turning the power off
bool scrTurnPowerOn(void)
{

	//powerCalculated = true;
	powerCalc(true);

	return true;
}

// -----------------------------------------------------------------------------------------
//flags when the tutorial is over so that console messages can be turned on again
bool scrTutorialEnd(void)
{
	initConsoleMessages();
	return true;
}

// -----------------------------------------------------------------------------------------
//function to play a full-screen video in the middle of the game for the selected player
bool scrPlayVideo(void)
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
//checks to see if there are any droids for the specified player
bool scrAnyDroidsLeft(void)
{
	SDWORD		player;
	bool		droidsLeft;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAnyUnitsLeft:player number is too high" );
		return false;
	}

	//check the players list for any droid
	droidsLeft = true;
	if (apsDroidLists[player] == NULL)
	{
		droidsLeft = false;
	}

	scrFunctionResult.v.bval = droidsLeft;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//function to call when the game is over, plays a message then does game over stuff.
//
bool scrGameOverMessage(void)
{
	int32_t			gameWon;	// was BOOL (int) ** see warning about conversion
	MESSAGE			*psMessage;
	MESSAGE_TYPE		msgType;
	SDWORD			player;
	VIEWDATA		*psViewData;

	if (!stackPopParams(4, ST_INTMESSAGE, &psViewData , VAL_INT, &msgType,
				VAL_INT, &player, VAL_BOOL, &gameWon))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGameOverMessage:player number is too high" );
		return false;
	}

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

	ASSERT( msgType != MSG_PROXIMITY, "scrGameOverMessage: Bad message type (MSG_PROXIMITY)" );

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
		updateChallenge(gameWon);
	}

	return true;
}


// -----------------------------------------------------------------------------------------
//function to call when the game is over
bool scrGameOver(void)
{
	int32_t	gameOver;	// was BOOL (int) ** see warning about conversion

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
bool scrAnyFactoriesLeft(void)
{
	SDWORD		player;
	bool		bResult;
	STRUCTURE	*psCurr;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAnyFactorysLeft:player number is too high" );
		return false;
	}

	//check the players list for any structures
	bResult = false;
	if(apsStructLists[player])
	{
		for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
		{
//			if (psCurr->pStructureType->type	== REF_FACTORY ||
//				psCurr->pStructureType->type == REF_CYBORG_FACTORY ||
//				psCurr->pStructureType->type == REF_VTOL_FACTORY )
			if(StructIsFactory(psCurr))
			{
				bResult = true;
				break;
			}
		}
	}

	scrFunctionResult.v.bval = bResult;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
//checks to see if there are any structures (except walls) for the specified player
bool scrAnyStructButWallsLeft(void)
{
	SDWORD		player;
	bool		structuresLeft;
	STRUCTURE	*psCurr;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAnyStructuresButWallsLeft:player number is too high" );
		return false;
	}

	//check the players list for any structures
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

	scrFunctionResult.v.bval = structuresLeft;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
//defines the background audio to play
bool scrPlayBackgroundAudio(void)
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

bool scrPlayIngameCDAudio(void)
{
	debug(LOG_SOUND, "Script wanted music to start");
	cdAudio_PlayTrack(SONG_INGAME);

	return true;
}

// -----------------------------------------------------------------------------------------
bool scrStopCDAudio(void)
{
	debug(LOG_SOUND, "Script wanted music to stop");
	cdAudio_Stop();
	return true;
}

// -----------------------------------------------------------------------------------------
bool scrPauseCDAudio(void)
{
	cdAudio_Pause();
	return true;
}

// -----------------------------------------------------------------------------------------
bool scrResumeCDAudio(void)
{
	cdAudio_Resume();
	return true;
}

// -----------------------------------------------------------------------------------------
// set the retreat point for a player
bool scrSetRetreatPoint(void)
{
	SDWORD	player, x,y;

	if (!stackPopParams(3, VAL_INT, &player, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetRetreatPoint: player out of range" );
		return false;
	}
	if (x < 0 || x >= (SDWORD)mapWidth*TILE_UNITS ||
		y < 0 || y >= (SDWORD)mapHeight*TILE_UNITS)
	{
		ASSERT( false, "scrSetRetreatPoint: coords off map" );
		return false;
	}

	asRunData[player].sPos.x = x;
	asRunData[player].sPos.y = y;

	return true;
}

// -----------------------------------------------------------------------------------------
// set the retreat force level
bool scrSetRetreatForce(void)
{
	SDWORD	player, level, numDroids;
	DROID	*psCurr;

	if (!stackPopParams(2, VAL_INT, &player, VAL_INT, &level))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetRetreatForce: player out of range" );
		return false;
	}

	if (level > 100 || level < 0)
	{
		ASSERT( false, "scrSetRetreatForce: level out of range" );
		return false;
	}

	// count up the current number of droids
	numDroids = 0;
	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		numDroids += 1;
	}

	asRunData[player].forceLevel = (UBYTE)(level * numDroids / 100);

	return true;
}

// -----------------------------------------------------------------------------------------
// set the retreat leadership
bool scrSetRetreatLeadership(void)
{
	SDWORD	player, level;

	if (!stackPopParams(2, VAL_INT, &player, VAL_INT, &level))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetRetreatLeadership: player out of range" );
		return false;
	}

	if (level > 100 || level < 0)
	{
		ASSERT( false, "scrSetRetreatLeadership: level out of range" );
		return false;
	}

	asRunData[player].leadership = (UBYTE)level;

	return true;
}

// -----------------------------------------------------------------------------------------
// set the retreat point for a group
bool scrSetGroupRetreatPoint(void)
{
	SDWORD		x,y;
	DROID_GROUP	*psGroup;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}

	if (x < 0 || x >= (SDWORD)mapWidth*TILE_UNITS ||
		y < 0 || y >= (SDWORD)mapHeight*TILE_UNITS)
	{
		ASSERT( false, "scrSetRetreatPoint: coords off map" );
		return false;
	}

	psGroup->sRunData.sPos.x = x;
	psGroup->sRunData.sPos.y = y;

	return true;
}

// -----------------------------------------------------------------------------------------
bool scrSetGroupRetreatForce(void)
{
	SDWORD		level, numDroids;
	DROID_GROUP	*psGroup;
	DROID		*psCurr;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &level))
	{
		return false;
	}

	if (level > 100 || level < 0)
	{
		ASSERT( false, "scrSetRetreatForce: level out of range" );
		return false;
	}

	// count up the current number of droids
	numDroids = 0;
	for(psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		numDroids += 1;
	}

	psGroup->sRunData.forceLevel = (UBYTE)(level * numDroids / 100);

	return true;
}

// -----------------------------------------------------------------------------------------
// set the retreat health level
bool scrSetRetreatHealth(void)
{
	SDWORD	player, health;

	if (!stackPopParams(2, VAL_INT, &player, VAL_INT, &health))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
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
bool scrSetGroupRetreatHealth(void)
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
// set the retreat leadership
bool scrSetGroupRetreatLeadership(void)
{
	SDWORD		level;
	DROID_GROUP	*psGroup;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &level))
	{
		return false;
	}

	if (level > 100 || level < 0)
	{
		ASSERT( false, "scrSetRetreatLeadership: level out of range" );
		return false;
	}

	psGroup->sRunData.leadership = (UBYTE)level;

	return true;
}

// -----------------------------------------------------------------------------------------
//start a Mission - the missionType is ignored now - gets it from the level data ***********
bool scrStartMission(void)
{
	char				*pGame;
	SDWORD				missionType;
	LEVEL_DATASET		*psNewLevel;

	if (!stackPopParams(2, VAL_INT, &missionType, ST_LEVEL, &pGame))
	{
		return false;
	}

	//if (missionType > MISSION_NONE)
	if (missionType > LDS_NONE)
	{
		ASSERT( false, "Invalid Mission Type" );
		return false;
	}

	sstrcpy(aLevelName, pGame);

	// find the level dataset
	psNewLevel = levFindDataSet(pGame);
	if (psNewLevel == NULL)
	{
		debug( LOG_FATAL, "scrStartMission: couldn't find level data" );
		abort();
		return false;
	}

	//set the mission rolling...
	//nextMissionType = missionType;
	nextMissionType = psNewLevel->type;
	loopMissionState = LMS_CLEAROBJECTS;

	return true;
}

// -----------------------------------------------------------------------------------------
//set Snow (enable disable snow)
bool scrSetSnow(void)
{
	int32_t bState;		// was BOOL (int) ** see warning about conversion

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
bool scrSetRain(void)
{
	int32_t bState;		// was BOOL (int) ** see warning about conversion

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
//set Background Fog (replace fade out with fog)
bool scrSetBackgroundFog(void)
{
	int32_t bState;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}

	// no-op

	return true;
}

// -----------------------------------------------------------------------------------------
//set Depth Fog (gradual fog from mid range to edge of world)
bool scrSetDepthFog(void)
{
	int32_t bState;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}

	// no-op

	return true;
}

// -----------------------------------------------------------------------------------------
//set Mission Fog colour, may be modified by weather effects
bool scrSetFogColour(void)
{
	SDWORD	red,green,blue;
	PIELIGHT scrFogColour;

	if (!stackPopParams(3, VAL_INT, &red, VAL_INT, &green, VAL_INT, &blue))
	{
		return false;
	}

	scrFogColour.byte.r = red;
	scrFogColour.byte.g = green;
	scrFogColour.byte.b = blue;
	scrFogColour.byte.a = 255;
	pie_SetFogColour(scrFogColour);

	return true;
}

// -----------------------------------------------------------------------------------------
// test function to test variable references
bool scrRefTest(void)
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
// is player a human or computer player? (multiplayer only)
bool scrIsHumanPlayer(void)
{
	SDWORD	player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	scrFunctionResult.v.bval = isHumanPlayer(player);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// Set an alliance between two players
bool scrCreateAlliance(void)
{
	SDWORD	player1,player2;

	if (!stackPopParams(2, VAL_INT, &player1, VAL_INT, &player2))
	{
		return false;
	}

	if (player1 < 0 || player1 >= MAX_PLAYERS ||
		player2 < 0 || player2 >= MAX_PLAYERS)
	{
		ASSERT( false, "scrCreateAlliance: player out of range p1=%d p2=%d", player1, player2 );
		return false;
	}

	if(bMultiPlayer)
	{
		if(game.alliance==NO_ALLIANCES || game.alliance==ALLIANCES_TEAMS
			|| player1 >= game.maxPlayers || player2>=game.maxPlayers)
		{
			return true;
		}
	}

	formAlliance((UBYTE)player1, (UBYTE)player2,true,false,true);

	return true;
}

// -----------------------------------------------------------------------------------------
// offer an alliance
bool scrOfferAlliance(void)
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
bool scrBreakAlliance(void)
{
	SDWORD	player1,player2;

	if (!stackPopParams(2, VAL_INT, &player1, VAL_INT, &player2))
	{
		return false;
	}

	if (player1 < 0 || player1 >= MAX_PLAYERS || player2 < 0 || player2 >= MAX_PLAYERS)
	{
		ASSERT( false, "scrBreakAlliance: player out of range p1=%d p2=%d", player1, player2 );
		return false;
	}

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

	return true;
}

// -----------------------------------------------------------------------------------------
// Multiplayer relevant scriptfuncs
// returns true if 2 or more players are in alliance.
bool scrAllianceExists(void)
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

bool scrAllianceExistsBetween(void)
{
	UDWORD i,j;

	if (!stackPopParams(2, VAL_INT, &i,VAL_INT, &j))
	{
		return false;
	}
	if (i < MAX_PLAYERS && j < MAX_PLAYERS && alliances[i][j] == ALLIANCE_FORMED)
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

// -----------------------------------------------------------------------------------------
bool scrPlayerInAlliance(void)
{
	UDWORD player,j;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	for(j=0;j<MAX_PLAYERS;j++)
	{
		if(alliances[player][j] == ALLIANCE_FORMED)
		{
			scrFunctionResult.v.bval = true;
			if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
			{
				return false;
			}
			return true;
		}
	}
	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------
// returns true if a single alliance is dominant.
bool scrDominatingAlliance(void)
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
	}


	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


bool scrMyResponsibility(void)
{
	SDWORD player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if(	myResponsibility(player) )
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
/**
 * Checks to see if a structure of the type specified exists within the specified range of an XY location.
 * Use player -1 to find structures owned by any player. In this case, ignore if they are completed.
 */
bool scrStructureBuiltInRange(void)
{
	SDWORD		player, index, x, y, range;
	BASE_OBJECT	*psCurr;
	STRUCTURE	*psStruct = NULL;
	STRUCTURE_STATS *psTarget;

	if (!stackPopParams(5, ST_STRUCTURESTAT, &index, VAL_INT, &x, VAL_INT, &y, VAL_INT, &range, VAL_INT, &player))
	{
		return false;
	}

	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Player index out of bounds");
	ASSERT_OR_RETURN(false, x >= 0 && map_coord(x) < mapWidth, "Invalid X coord");
	ASSERT_OR_RETURN(false, y >= 0 && map_coord(y) < mapHeight, "Invalid Y coord");
	ASSERT_OR_RETURN(false, index >= 0 && index < numStructureStats, "Invalid structure stat");
	ASSERT_OR_RETURN(false, range >= 0, "Negative range");

	// Now look through the players list of structures to see if this type exists within range
	psTarget = &asStructureStats[index];

	gridStartIterate(x, y, range);
	for (psCurr = gridIterate(); psCurr; psCurr = gridIterate())
	{
		if (psCurr->type == OBJ_STRUCTURE)
		{
			psStruct = (STRUCTURE *)psCurr;
			if ((psStruct->status == SS_BUILT || player == -1)
			    && (player == -1 || psStruct->player == player)
			    && strcmp(psStruct->pStructureType->pName, psTarget->pName) == 0)
			{
				break;
			}
		}
		psStruct = NULL;
	}

	scrFunctionResult.v.oval = psStruct;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------------------
// generate a random number
bool scrRandom(void)
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
bool scrRandomiseSeed(void)
{
	// Why? What's the point? What on earth were they thinking, exactly? If the numbers don't have enough randominess, just set the random seed again and again until the numbers are double-plus super-duper full of randonomium?
	debug(LOG_ERROR, "A script is trying to set the random seed with srand(). That just doesn't make sense.");
	//srand((UDWORD)clock());

	// Resisting the urge to return a random number here. Afraid of triggering some sort of fallback mechanism in the scripts for when setting the random seed somehow fails.
	return true;
}

// -----------------------------------------------------------------------------------------
//explicitly enables a research topic
bool scrEnableResearch(void)
{
	SDWORD		player;
	RESEARCH	*psResearch;

	if (!stackPopParams(2, ST_RESEARCH, &psResearch, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrEnableResearch:player number is too high" );
		return false;
	}

	if (!enableResearch(psResearch, player))
	{
		return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------
//acts as if the research topic was completed - used to jump into the tree
bool scrCompleteResearch(void)
{
	SDWORD		player;
	RESEARCH	*psResearch;
	UDWORD		researchIndex;

	if (!stackPopParams(2, ST_RESEARCH, &psResearch, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrCompleteResearch:player number is too high" );
		return false;
	}

	if (psResearch == NULL)
	{
		// hack to make T2 and T3 work even though the research lists 
		// are polluted with tons of non-existent techs :(
		return true;
	}

	researchIndex = psResearch->index;
	if (researchIndex > asResearch.size())
	{
		ASSERT( false, "scrCompleteResearch: invalid research index" );
		return false;
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


	return true;
}

// -----------------------------------------------------------------------------------------
// This routine used to start just a reticule button flashing
//   .. now it starts any button flashing (awaiting implmentation from widget library)
bool scrFlashOn(void)
{
	SDWORD		button;

	if (!stackPopParams(1, VAL_INT, &button))
	{
		return false;
	}

	// For the time being ... we will perform the old code for the reticule ...
	if (button >= IDRET_OPTIONS && button <= IDRET_CANCEL)
	{
		flashReticuleButton((UDWORD)button);
		return true;
	}


	if(widgGetFromID(psWScreen,button) != NULL)
	{
		widgSetButtonFlash(psWScreen,button);
	}
	return true;
}


// -----------------------------------------------------------------------------------------
// stop a generic button flashing
bool scrFlashOff(void)
{
	SDWORD		button;

	if (!stackPopParams(1, VAL_INT, &button))
	{
		return false;
	}

	if (button >= IDRET_OPTIONS && button <= IDRET_CANCEL)
	{
		stopReticuleButtonFlash((UDWORD)button);
		return true;
	}


	if(widgGetFromID(psWScreen,button) != NULL)
	{
		widgClearButtonFlash(psWScreen,button);
	}
	return true;
}

// -----------------------------------------------------------------------------------------
//set the initial power level settings for a player
bool scrSetPowerLevel(void)
{
	SDWORD		player, power;

	if (!stackPopParams(2, VAL_INT, &power, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetPowerLevel:player number is too high" );
		return false;
	}

	setPower(player, power);

	return true;
}

// -----------------------------------------------------------------------------------------
//add some power for a player
bool scrAddPower(void)
{
	SDWORD		player, power;

	if (!stackPopParams(2, VAL_INT, &power, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrAddPower:player number is too high" );
		return false;
	}

	giftPower(ANYPLAYER, player, power, true);

	return true;
}

// -----------------------------------------------------------------------------------------
/*set the landing Zone position for the map - this is for player 0. Can be
scrapped and replaced by setNoGoAreas, left in for compatibility*/
bool scrSetLandingZone(void)
{
	SDWORD		x1, x2, y1, y2;

	if (!stackPopParams(4, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	//check the values - check against max possible since can set in one mission for the next
	if (x1 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLandingZone: x1 is greater than max mapWidth" );
		return false;
	}
	if (x2 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLandingZone: x2 is greater than max mapWidth" );
		return false;
	}
	if (y1 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetLandingZone: y1 is greater than max mapHeight" );
		return false;
	}
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

	setNoGoArea((UBYTE)x1, (UBYTE)y1, (UBYTE)x2, (UBYTE)y2, 0);

	return true;
}

/*set the landing Zone position for the Limbo droids and adds the Limbo droids
to the world at the location*/
bool scrSetLimboLanding(void)
{
	SDWORD		x1, x2, y1, y2;

	if (!stackPopParams(4, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	//check the values - check against max possible since can set in one mission for the next
	if (x1 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLimboLanding: x1 is greater than max mapWidth" );
		return false;
	}
	if (x2 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetLimboLanding: x2 is greater than max mapWidth" );
		return false;
	}
	if (y1 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetLimboLanding: y1 is greater than max mapHeight" );
		return false;
	}
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

	// this calls the Droids from the Limbo list onto the map
	placeLimboDroids();

	return true;
}

// -----------------------------------------------------------------------------------------
//initialises all the no go areas
bool scrInitAllNoGoAreas(void)
{
	initNoGoAreas();

	return true;
}

// -----------------------------------------------------------------------------------------
//set a no go area for the map - landing zones for the enemy, or player 0
bool scrSetNoGoArea(void)
{
	SDWORD		x1, x2, y1, y2, area;

	if (!stackPopParams(5, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2,
		VAL_INT, &area))
	{
		return false;
	}

    if (area == LIMBO_LANDING)
    {
        ASSERT( false, "scrSetNoGoArea: Cannot set the Limbo Landing area with this function" );
        return false;
    }

	//check the values - check against max possible since can set in one mission for the next
	//if (x1 > (SDWORD)mapWidth)
	if (x1 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetNoGoArea: x1 is greater than max mapWidth" );
		return false;
	}
	//if (x2 > (SDWORD)mapWidth)
	if (x2 > (SDWORD)MAP_MAXWIDTH)
	{
		ASSERT( false, "scrSetNoGoArea: x2 is greater than max mapWidth" );
		return false;
	}
	//if (y1 > (SDWORD)mapHeight)
	if (y1 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetNoGoArea: y1 is greater than max mapHeight" );
		return false;
	}
	//if (y2 > (SDWORD)mapHeight)
	if (y2 > (SDWORD)MAP_MAXHEIGHT)
	{
		ASSERT( false, "scrSetNoGoArea: y2 is greater than max mapHeight" );
		return false;
	}
	//check won't overflow!
	if (x1 > UBYTE_MAX || y1 > UBYTE_MAX || x2 > UBYTE_MAX || y2 > UBYTE_MAX)
	{
		ASSERT( false, "scrSetNoGoArea: one coord is greater than %d", UBYTE_MAX );
		return false;
	}

	if (area >= MAX_NOGO_AREAS)
	{
		ASSERT( false, "scrSetNoGoArea: max num of areas is %d", MAX_NOGO_AREAS );
		return false;
	}

	setNoGoArea((UBYTE)x1, (UBYTE)y1, (UBYTE)x2, (UBYTE)y2, (UBYTE)area);

	return true;
}


// -----------------------------------------------------------------------------------------
// set the zoom level for the radar
bool scrSetRadarZoom(void)
{
	SDWORD	level;

	if (!stackPopParams(1, VAL_INT, &level))
	{
		return true;
	}

	if (level < 0 || level > 2)
	{
		ASSERT( false, "scrSetRadarZoom: zoom level out of range" );
		return false;
	}

	SetRadarZoom((UWORD)level);

	return true;
}

// -----------------------------------------------------------------------------------------
//set how long an offworld mission can last -1 = no limit
bool scrSetMissionTime(void)
{
	SDWORD		time;

	if (!stackPopParams(1, VAL_INT, &time))
	{
		return false;
	}

	time *= 100;

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
		// ffs ab    ... but shouldn't this be on the psx ?
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

	return true;
}

// this returns how long is left for the current mission time is 1/100th sec - same units as passed in
bool scrMissionTimeRemaining(void)
{
    SDWORD      timeRemaining;

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
//set the time delay for reinforcements for an offworld mission
bool scrSetReinforcementTime(void)
{
	SDWORD		time;
    DROID       *psDroid;

	if (!stackPopParams(1, VAL_INT, &time))
	{
		return false;
	}

    time *= 100;

	//check not more than one hour - the mission timers cannot cope at present!
	if (time != LZ_COMPROMISED_TIME && time > 60*60*GAME_TICKS_PER_SEC)
	{
		ASSERT( false,"The transport timer cannot be set to more than 1 hour!" );
		time = -1;
	}

	//store the value
	mission.ETA = time;

	//if offworld or campaign change mission, then add the timer
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
            if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
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

	return true;
}

// -----------------------------------------------------------------------------------------
// Sets all structure limits for a player to a specified value
bool scrSetAllStructureLimits(void)
{
	SDWORD				player, limit;
	STRUCTURE_LIMITS	*psStructLimits;
	UDWORD				i;

	if (!stackPopParams(2, VAL_INT, &limit, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
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
		psStructLimits[i].limit = limit;

		psStructLimits[i].globalLimit = limit;

	}

	return true;
}


// -----------------------------------------------------------------------------------------
// clear all the console messages
bool scrFlushConsoleMessages(void)
{
	flushConsoleMessages();

	return true;
}

// -----------------------------------------------------------------------------------------
// Establishes the distance between two points - uses an approximation
bool scrDistanceTwoPts( void )
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
bool	scrLOSTwoBaseObjects( void )
{
BASE_OBJECT	*psSource,*psDest;
int32_t		bWallsBlock;		// was BOOL (int) ** see warning about conversion
bool		retVal;

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
// Destroys all structures within a certain bounding area.
bool	scrDestroyStructuresInArea( void )
{
SDWORD		x1,y1,x2,y2;
UDWORD		typeRef;
UDWORD		player;
STRUCTURE	*psStructure,*psNextS;
FEATURE		*psFeature,*psNextF;
int32_t		bVisible,bTakeFeatures;		// was BOOL (int) ** see warning about conversion
SDWORD		sX,sY;

	if(!stackPopParams(8, VAL_INT, &player, VAL_INT, &typeRef, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2,
						VAL_INT, &y2, VAL_BOOL, &bVisible, VAL_BOOL, &bTakeFeatures))
	{
		ASSERT( false,"SCRIPT : scrDestroyStructuresInArea - Cannot get parameters" );
		return(false);
	}

	if(player>=MAX_PLAYERS)
	{
		ASSERT( false,"Player number too high in scrDestroyStructuresInArea" );
	}

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
						destroyFeature(psFeature, gameTime);
					}
					else
					{
						removeFeature(psFeature);
					}
				}
			}
		}
	}
	return(true);
}
// -----------------------------------------------------------------------------------------
// Returns a value representing the threat from droids in a given area
bool	scrThreatInArea( void )
{
SDWORD	x1,y1,x2,y2;
SDWORD	ldThreat,mdThreat,hdThreat;
UDWORD	playerLooking,playerTarget;
SDWORD	totalThreat;
DROID	*psDroid;
SDWORD	dX,dY;
int32_t	bVisible;		// was BOOL (int) ** see warning about conversion

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
bool scrGetNearestGateway( void )
{
	SDWORD	x,y;
	UDWORD	nearestSoFar;
	UDWORD	dist;
	GATEWAY	*psGateway;
	SDWORD	retX,retY;
	SDWORD	*rX,*rY;
	bool	success;

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
bool	scrSetWaterTile(void)
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
bool	scrSetRubbleTile(void)
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
bool	scrSetCampaignNumber(void)
{
UDWORD	campaignNumber;

	if(!stackPopParams(1,VAL_INT, &campaignNumber))
	{
		ASSERT( false,"SCRIPT : Cannot get parameter for scrSetCampaignNumber" );
		return(false);
	}


	setCampaignNumber(campaignNumber);

	return(true);
}

// -----------------------------------------------------------------------------------------
// Tests whether a structure has a certain module for a player. Tests whether any structure
// has this module if structure is null
bool	scrTestStructureModule(void)
{
SDWORD	player,refId;
STRUCTURE	*psStructure,*psStruct;
bool	bFound;

	if(!stackPopParams(3,VAL_INT,&player,ST_STRUCTURE,&psStructure,VAL_INT,&refId))
	{
		ASSERT( false,"SCRIPT : Cannot get parameters in scrTestStructureModule" );
		return(false);
	}

	if(player>=MAX_PLAYERS)
	{
		ASSERT( false,"SCRIPT : Player number too high in scrTestStructureModule" );
		return(false);

	}

	/* Nothing yet */
	bFound = false;

	/* Check the specified case first */
	if(psStructure)
	{
		if(structHasModule(psStructure))
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
			if(structHasModule(psStruct))
			{
				bFound = true;
			}
		}
	}

	/* Send back the scrFunctionResult */
	scrFunctionResult.v.bval = bFound;
	if(!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		ASSERT( false,"SCRIPT : Cannot push scrFunctionResult for scrTestStructureModule" );
		return(false);
	}

	return(true);
}


// -----------------------------------------------------------------------------------------
bool	scrForceDamage( void )
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
bool	scrDestroyUnitsInArea( void )
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

	if(player>=MAX_PLAYERS)
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
bool	scrRemoveDroid( void )
{
DROID	*psDroid;

	if(!stackPopParams(1,ST_DROID,&psDroid))
	{		ASSERT( false,"Cannot get vars for scrRemoveDroid!" );
		return(false);
	}

	if(psDroid)
	{
		vanishDroid(psDroid);
	}

	return(true);
}

// -----------------------------------------------------------------------------------------
static bool	structHasModule(STRUCTURE *psStruct)
{
	STRUCTURE_STATS	*psStats;
	bool bFound = false;

	ASSERT_OR_RETURN(false, psStruct != NULL, "Testing for a module from a NULL struct");

	/* Fail if the structure isn't built yet */
	if(psStruct->status != SS_BUILT)
	{
		return(false);
	}

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

	return(bFound);
}

// -----------------------------------------------------------------------------------------
// give player a template belonging to another.
bool scrAddTemplate(void)
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
bool structDoubleCheck(BASE_STATS *psStat,UDWORD xx,UDWORD yy, SDWORD maxBlockingTiles)
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

static bool pickStructLocation(DROID *psDroid, int index, int *pX, int *pY, int player, int maxBlockingTiles)
{
	STRUCTURE_STATS	*psStat = &asStructureStats[index];
	UDWORD			numIterations = 30;
	bool			found = false;
	int startX, startY, incX, incY, x, y;

	ASSERT_OR_RETURN(false, player < MAX_PLAYERS && player >= 0, "Invalid player number %d", player);

	Vector2i offset(psStat->baseWidth * (TILE_UNITS / 2), psStat->baseBreadth * (TILE_UNITS / 2));  // This gets added to the chosen coordinates, at the end of the function.

	// check for wacky coords.
	if (*pX < 0 || *pX > world_coord(mapWidth) || *pY < 0 || *pY > world_coord(mapHeight))
	{
		debug(LOG_ERROR, "Bad parameters");
		goto endstructloc;
	}

	startX = map_coord(*pX);				// change to tile coords.
	startY = map_coord(*pY);
	x = startX;
	y = startY;

	// save a lot of typing... checks whether a position is valid
	#define LOC_OK(_x, _y) (tileOnMap(_x, _y) && \
				(!psDroid || fpathCheck(psDroid->pos, Vector3i(world_coord(_x), world_coord(_y), 0), PROPULSION_TYPE_WHEELED)) \
				&& validLocation(psStat, world_coord(Vector2i(_x, _y)) + offset, 0, player, false) && structDoubleCheck(psStat, _x, _y, maxBlockingTiles))

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
		*pX = world_coord(x) + offset.x;
		*pY = world_coord(y) + offset.y;
	}
	else
	{
		debug(LOG_SCRIPT, "Did not find valid positioning for %s", psStat->pName);
	}
	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// success!
	{
		return false;
	}
	return true;
}

// pick a structure location(only used in skirmish game at 27Aug) ajl.
bool scrPickStructLocation(void)
{
	SDWORD			*pX,*pY;
	SDWORD			index;
	UDWORD			player;

	if (!stackPopParams(4, ST_STRUCTURESTAT, &index, VAL_REF|VAL_INT, &pX, VAL_REF|VAL_INT, &pY, VAL_INT, &player))
	{
		return false;
	}
	return pickStructLocation(NULL, index, pX, pY, player, MAX_BLOCKING_TILES);
}

// pick a structure location and check that we can build there (duh!)
bool scrPickStructLocationC(void)
{
	int			*pX, *pY, index, player, maxBlockingTiles;
	DROID			*psDroid;

	if (!stackPopParams(6, ST_DROID, &psDroid, ST_STRUCTURESTAT, &index, VAL_REF|VAL_INT, &pX , VAL_REF|VAL_INT, &pY, VAL_INT, &player, VAL_INT, &maxBlockingTiles))
	{
		return false;
	}
	return pickStructLocation(psDroid, index, pX, pY, player, maxBlockingTiles);
}

// pick a structure location(only used in skirmish game at 27Aug) ajl.
// Max number of blocking tiles is passed as parameter for this one
bool scrPickStructLocationB(void)
{
	SDWORD			*pX,*pY;
	SDWORD			index;
	UDWORD			player;
	SDWORD			maxBlockingTiles;

	if (!stackPopParams(5, ST_STRUCTURESTAT, &index, VAL_REF|VAL_INT, &pX ,
        VAL_REF|VAL_INT, &pY, VAL_INT, &player, VAL_INT, &maxBlockingTiles))
	{
		return false;
	}
	return pickStructLocation(NULL, index, pX, pY, player, maxBlockingTiles);
}

// -----------------------------------------------------------------------------------------
// Sets the transporter entry and exit points for the map
bool scrSetTransporterExit(void)
{
	SDWORD	iPlayer, iExitTileX, iExitTileY;

	if (!stackPopParams(3, VAL_INT, &iPlayer, VAL_INT, &iExitTileX, VAL_INT, &iExitTileY))
	{
		return false;
	}

	missionSetTransporterExit( iPlayer, iExitTileX, iExitTileY );

	return true;
}

// -----------------------------------------------------------------------------------------
// Fly transporters in at start of map
bool scrFlyTransporterIn(void)
{
	SDWORD	iPlayer, iEntryTileX, iEntryTileY;
	int32_t	bTrackTransporter;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(4, VAL_INT, &iPlayer, VAL_INT, &iEntryTileX, VAL_INT, &iEntryTileY,
							VAL_BOOL, &bTrackTransporter))
	{
		return false;
	}

	missionSetTransporterEntry( iPlayer, iEntryTileX, iEntryTileY );
	missionFlyTransportersIn( iPlayer, bTrackTransporter );

	return true;
}

// -----------------------------------------------------------------------------------------



/*
 ** scrGetGameStatus
 *
 *  PARAMETERS: The parameter passed must be one of the STATUS_ variable
 *
 *  DESCRIPTION: Returns various bool options in the game	e.g. If the reticule is open
 *      - You should use the externed variable intMode for other game mode options
 *        e.g. in the intelligence screen or desgin screen)
 *
 *  RETURNS:
 *
 */
bool scrGetGameStatus(void)
{
	SDWORD GameChoice;
	bool bResult;

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
			if (deliveryReposValid())
				bResult=true;
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
bool scrGetPlayerColour(void)
{
	SDWORD		player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
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

//get the colour name of the player ("green", "black" etc)
bool scrGetPlayerColourName(void)
{
	SDWORD		player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS || player < 0)
	{
		ASSERT( false, "scrGetPlayerColourName: wrong player index" );
		return false;
	}

	/* Casting away constness because stackPushResult doesn't modify it's
	 * value (i.e. in this case it's not const correct).
	 */
	scrFunctionResult.v.sval = (char*)getPlayerColourName(player);
	if (!stackPushResult(VAL_STRING, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetPlayerColourName(): failed to push result");
		return false;
	}

	return true;
}

//set the colour number to use for a player
bool scrSetPlayerColour(void)
{
	SDWORD		player, colour;

	if (!stackPopParams(2, VAL_INT, &colour, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetPlayerColour:player number is too high" );
		return false;
	}

	if (colour >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetPlayerColour:colour number is too high" );
		return false;
	}

    //not the end of the world if this doesn't work so don't check the return code
    (void)setPlayerColour(player, colour);

	return true;
}

//set all droids in an area to belong to a different player - returns the number of droids changed
bool scrTakeOverDroidsInArea(void)
{
	SDWORD		fromPlayer, toPlayer, x1, x2, y1, y2, numChanged;
	DROID       *psDroid, *psNext;

	if (!stackPopParams(6, VAL_INT, &fromPlayer, VAL_INT, &toPlayer, VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, fromPlayer < MAX_PLAYERS && toPlayer < MAX_PLAYERS, "player number is too high");
	ASSERT_OR_RETURN(false, x1 < world_coord(MAP_MAXWIDTH) && x2 < world_coord(MAP_MAXWIDTH) && y1 < world_coord(MAP_MAXHEIGHT)
	                        && y2 < world_coord(MAP_MAXHEIGHT), "coordinate outside map");
	numChanged = 0;
	for (psDroid = apsDroidLists[fromPlayer]; psDroid != NULL; psDroid = psNext)
	{
		psNext = psDroid->psNext;
		// check if within area specified
		if (psDroid->pos.x >= x1 && psDroid->pos.x <= x2 && psDroid->pos.y >= y1 && psDroid->pos.y <= y2
		    && psDroid->player != toPlayer)
		{
			if (giftSingleDroid(psDroid, toPlayer))	// give the droid away
			{
				numChanged++;
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

/*this takes over a single droid and passes a pointer back to the new one*/
bool scrTakeOverSingleDroid(void)
{
	SDWORD			playerToGain;
    DROID           *psDroidToTake, *psNewDroid;

    if (!stackPopParams(2, ST_DROID, &psDroidToTake, VAL_INT, &playerToGain))
    {
		return false;
    }

	if (playerToGain >= MAX_PLAYERS)
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
bool scrTakeOverDroidsInAreaExp(void)
{
	SDWORD		fromPlayer, toPlayer, x1, x2, y1, y2, numChanged, level, maxUnits;
    DROID       *psDroid, *psNext;

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
bool scrTakeOverSingleStructure(void)
{
	SDWORD			playerToGain;
    STRUCTURE       *psStructToTake, *psNewStruct;
    UDWORD          structureInc;

    if (!stackPopParams(2, ST_STRUCTURE, &psStructToTake, VAL_INT, &playerToGain))
    {
		return false;
    }

	if (playerToGain >= MAX_PLAYERS)
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
bool scrTakeOverStructsInArea(void)
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
bool scrSetDroidsToSafetyFlag(void)
{
	int32_t bState;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}

    setDroidsToSafetyFlag(bState);

	return true;
}

//set Flag for defining whether the coded countDown is called
bool scrSetPlayCountDown(void)
{
	int32_t bState;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(1, VAL_BOOL, &bState))
	{
		return false;
	}


    setPlayCountDown((UBYTE)bState);


	return true;
}

//get the number of droids currently onthe map for a player
bool scrGetDroidCount(void)
{
	SDWORD		player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGetUnitCount:player number is too high" );
		return false;
	}

	scrFunctionResult.v.ival = getNumDroids(player);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// fire a weapon stat at an object
bool scrFireWeaponAtObj(void)
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
bool scrFireWeaponAtLoc(void)
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
bool scrSetDroidKills(void)
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
bool scrGetDroidKills(void)
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
bool scrResetPlayerVisibility(void)
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
bool scrSetVTOLReturnPos(void)
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
bool scrResetLimboMission(void)
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



// skirmish only.
bool scrIsVtol(void)
{
	DROID *psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		return true;
	}

	if(psDroid == NULL)
	{
		ASSERT( false,"scrIsVtol: null droid passed in." );
	}

	scrFunctionResult.v.bval = isVtolDroid(psDroid) ;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

// Fix the tutorial's template list(s).
// DO NOT MODIFY THIS WITHOUT KNOWING WHAT YOU ARE DOING.  You will break the tutorial!
// In short, we want to design a ViperLtMGWheels, but it is already available to make, so we must delete it.
bool scrTutorialTemplates(void)
{
	DROID_TEMPLATE	*psCurr, *psPrev;

	// find ViperLtMGWheels
	char const *pName = getDroidResourceName("ViperLtMGWheels");

	for (psCurr = apsDroidTemplates[selectedPlayer], psPrev = NULL; psCurr != NULL;	psCurr = psCurr->psNext)
	{
		if (strcmp(pName, psCurr->aName)==0)
		{
			if (psPrev)
			{
				psPrev->psNext = psCurr->psNext;
			}
			else
			{
				apsDroidTemplates[selectedPlayer] = psCurr->psNext;
			}
			break;
		}
		psPrev = psCurr;
	}

	// Delete the template in *both* lists!
	if(psCurr)
	{
		for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
		{
			DROID_TEMPLATE *dropTemplate = &*i;
			if (psCurr->multiPlayerID == dropTemplate->multiPlayerID)
			{
				free(dropTemplate->pName);
				localTemplates.erase(i);
				break;
			}
		}
		delete psCurr;
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
bool scrStrcmp(void)
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
bool scrConsole(void)
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		debug(LOG_ERROR, "scrConsole(): stack failed");
		return false;
	}

	addConsoleMessage(strParam1,DEFAULT_JUSTIFY, SYSTEM_MESSAGE);

	return true;
}

int32_t scrDebug[MAX_PLAYERS];		// was BOOL (int) ** see warning about conversion

//turn on debug messages
bool scrDbgMsgOn(void)
{
	int32_t	bOn;		// was BOOL (int) ** see warning about conversion
	SDWORD	player;

	if (!stackPopParams(2, VAL_INT, &player, VAL_BOOL, &bOn))
	{
		debug(LOG_ERROR, "scrDbgMsgOn(): stack failed");
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrDbgMsgOn(): wrong player number");
		return false;
	}

	scrDebug[player] = bOn;

	return true;
}

bool scrMsg(void)
{
	SDWORD	playerTo,playerFrom;
	char tmp[255];

	if (!stackPopParams(3, VAL_STRING, &strParam1, VAL_INT, &playerFrom, VAL_INT, &playerTo))
	{
		debug(LOG_ERROR, "scrMsg(): stack failed");
		return false;
	}

	if(playerFrom < 0 || playerFrom >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrMsg(): playerFrom out of range");
		return false;
	}

	if(playerTo < 0 || playerTo >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrMsg(): playerTo out of range");
		return false;
	}

	ssprintf(tmp, "%d%s", playerTo, strParam1);
	sendTextMessage(tmp, false, playerFrom);

	//show the message we sent on our local console as well (even in skirmish, if player plays as this AI)
	if(playerFrom == selectedPlayer)
	{
		ssprintf(tmp, "[%d-%d] : %s", playerFrom, playerTo, strParam1);	// add message
		addConsoleMessage(tmp, RIGHT_JUSTIFY, playerFrom);
	}

	return true;
}

bool scrDbg(void)
{
	SDWORD	player;

	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrDbg(): stack failed");
		return false;
	}

	if(scrDebug[player])
	{
		char	sTmp[255];
		sprintf(sTmp,"%d) %s",player,strParam1);
		addConsoleMessage(sTmp,DEFAULT_JUSTIFY, player);
	}

	return true;
}

bool scrDebugFile(void)
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		debug(LOG_ERROR, "scrDebugFile(): stack failed");
		return false;
	}

	debug(LOG_SCRIPT, "%s", strParam1);

	return true;
}

static	UDWORD			playerToEnumDroid;
static	UDWORD			playerVisibleDroid;
static	UDWORD			enumDroidCount;

/* Prepare the droid iteration */
bool scrInitEnumDroids(void)
{
	SDWORD	targetplayer,playerVisible;

	if ( !stackPopParams(2,  VAL_INT, &targetplayer, VAL_INT, &playerVisible) )
	{
		//DbgMsg("scrInitEnumDroids() - failed to pop params");
		return false;
	}

	playerToEnumDroid	= (UDWORD)targetplayer;
	playerVisibleDroid	= (UDWORD)playerVisible;
	enumDroidCount = 0;		//returned 0 droids so far
	return true;
}

/* Get next droid */
bool scrEnumDroid(void)
{
	UDWORD			count;
	DROID		 *psDroid;

	count = 0;
	for(psDroid=apsDroidLists[playerToEnumDroid];psDroid && count<enumDroidCount;count++)
	{
		psDroid = psDroid->psNext;
	}

	//search the players' list of droid to see if one exists and is visible
	while(psDroid)
	{
		if(psDroid->visible[playerVisibleDroid])
		{
			scrFunctionResult.v.oval = psDroid;
			if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))			//	push scrFunctionResult
			{
				return false;
			}

			enumDroidCount++;
			return true;
		}

		enumDroidCount++;
		psDroid = psDroid->psNext;
	}

	// push NULLDROID, since didn't find any
	scrFunctionResult.v.oval = NULL;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrEnumDroid() - push failed");
		return false;
	}

	return true;
}

//Return the template factory is currently building
bool scrFactoryGetTemplate(void)
{
	STRUCTURE		*psStructure = NULL;
	DROID_TEMPLATE	*psTemplate = NULL;

	if (!stackPopParams(1, ST_STRUCTURE, &psStructure))
	{
		debug(LOG_ERROR, "scrFactoryGetTemplate() - stackPopParams failed");
		return false;
	}

	if (psStructure == NULL)
	{
		debug(LOG_ERROR, "scrFactoryGetTemplate() - NULL factory object");
		ASSERT( false, "scrFactoryGetTemplate: NULL factory object" );
		return false;
	}

	ASSERT( psStructure != NULL,
		"scrFactoryGetTemplate: Invalid structure pointer" );
	ASSERT( (psStructure->pStructureType->type == REF_FACTORY ||
		psStructure->pStructureType->type == REF_CYBORG_FACTORY ||
		psStructure->pStructureType->type == REF_VTOL_FACTORY),
		"scrFactoryGetTemplate: structure is not a factory" );

	if(!StructIsFactory(psStructure))
	{
		debug(LOG_ERROR, "scrFactoryGetTemplate: structure not a factory.");
		return false;
	}

	psTemplate = (DROID_TEMPLATE *)((FACTORY*)psStructure->pFunctionality)->psSubject;

	ASSERT( psTemplate != NULL,
		"scrFactoryGetTemplate: Invalid template pointer" );

	scrFunctionResult.v.oval = psTemplate;
	if (!stackPushResult((INTERP_TYPE)ST_TEMPLATE, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrFactoryGetTemplate: stackPushResult failed");
		return false;
	}

	return true;
}

bool scrNumTemplatesInProduction(void)
{
	SDWORD			player,numTemplates = 0;
	DROID_TEMPLATE	*psTemplate;
    STRUCTURE		*psStruct;
	STRUCTURE		*psList;
	BASE_STATS		*psBaseStats;

	if (!stackPopParams(2, ST_TEMPLATE, &psTemplate, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrNumTemplatesInProduction: stackPopParams failed");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrNumTemplatesInProduction: player number is too high");
		ASSERT( false, "scrNumTemplatesInProduction: player number is too high" );
		return false;
	}

	ASSERT( psTemplate != NULL,
		"scrNumTemplatesInProduction: Invalid template pointer" );

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

	scrFunctionResult.v.ival = numTemplates;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumTemplatesInProduction: stackPushResult failed");
		return false;
	}

	return true;
}

// Returns number of units based on a component a certain player has
bool scrNumDroidsByComponent(void)
{
	SDWORD				player,lookingPlayer,comp;
	UDWORD				numFound;
	INTERP_VAL			sVal;
	DROID				*psDroid;

	if (!stackPopParams(2, VAL_INT, &player, VAL_INT, &lookingPlayer))
	{
		debug(LOG_ERROR, "scrNumDroidsByComponent(): stack failed");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrNumDroidsByComponent(): player number is too high");
		ASSERT( false, "scrNumDroidsByComponent:player number is too high" );
		return false;
	}

	if (!stackPop(&sVal))
	{
		debug(LOG_ERROR, "scrNumDroidsByComponent(): failed to pop component");
		return false;
	}

	numFound = 0;

	comp = (SDWORD)sVal.v.ival;	 //cache access

	//check droids
	for(psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->visible[lookingPlayer])		//can see this droid?
		{
			switch ((unsigned)sVal.type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
			{
			case ST_BODY:
				if (psDroid->asBits[COMP_BODY].nStat == comp)
				{
					numFound++;
				}
				break;
			case ST_PROPULSION:
				if (psDroid->asBits[COMP_PROPULSION].nStat == comp)
				{
					numFound++;
				}
				break;
			case ST_ECM:
				if (psDroid->asBits[COMP_ECM].nStat == comp)
				{
					numFound++;
				}
				break;
			case ST_SENSOR:
				if (psDroid->asBits[COMP_SENSOR].nStat == comp)
				{
					numFound++;
				}
				break;
			case ST_CONSTRUCT:
				if (psDroid->asBits[COMP_CONSTRUCT].nStat == comp)
				{
					numFound++;
				}
				break;
			case ST_REPAIR:
				if (psDroid->asBits[COMP_REPAIRUNIT].nStat == comp)
				{
					numFound++;
				}
				break;
			case ST_WEAPON:
				if (psDroid->asWeaps[0].nStat == comp)
				{
					numFound++;
					break;
				}
				break;
			case ST_BRAIN:
				if (psDroid->asBits[COMP_BRAIN].nStat == comp)
				{
					numFound++;
				}
				break;
			default:
				debug(LOG_ERROR, "scrNumDroidsByComponent(): unknown component type");
				ASSERT( false, "scrNumDroidsByComponent: unknown component type" );
				return false;
			}
		}
	}

	scrFunctionResult.v.ival = numFound;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumDroidsByComponent(): stackPushResult failed");
		return false;
	}

	return true;
}

bool scrGetStructureLimit(void)
{
	SDWORD				player,limit;
	UDWORD				structInc;
	STRUCTURE_LIMITS	*psStructLimits;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &structInc, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrGetStructureLimit(): stackPopParams failed");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrGetStructureLimit(): player number is too high");
		ASSERT( false, "scrSetStructureLimits: player number is too high" );
		return false;}

	if (structInc > numStructureStats)
	{
		debug(LOG_ERROR, "scrGetStructureLimit(): tructure stat is too high - %d", structInc);
		ASSERT( false, "scrSetStructureLimits: Structure stat is too high - %d", structInc );
		return false;}

	psStructLimits = asStructLimits[player];
	limit = (SDWORD)psStructLimits[structInc].limit;

	scrFunctionResult.v.ival = limit;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetStructureLimit(): stackPushResult failed");
		return false;
	}

	return true;
}

// Returns true if limit for the passed structurestat is reached, otherwise returns false
bool scrStructureLimitReached(void)
{
	SDWORD				player;
	bool				bLimit = false;
	UDWORD				structInc;
	STRUCTURE_LIMITS	*psStructLimits;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &structInc, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrStructureLimitReached(): stackPopParams failed");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrStructureLimitReached(): player number is too high");
		ASSERT( false, "scrSetStructureLimits: player number is too high" );
		return false;
	}

	if (structInc > numStructureStats)
	{
		debug(LOG_ERROR, "scrStructureLimitReached(): Structure stat is too high - %d", structInc);
		ASSERT( false, "scrSetStructureLimits: Structure stat is too high - %d", structInc );
		return false;}

	psStructLimits = asStructLimits[player];

	if(psStructLimits[structInc].currentQuantity >= psStructLimits[structInc].limit) bLimit = true;

	scrFunctionResult.v.bval = bLimit;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrStructureLimitReached(): stackPushResult failed");
		return false;
	}

	return true;
}

// How many structures of a given type a player has
bool scrGetNumStructures(void)
{
	SDWORD				player,numStructures;
	UDWORD				structInc;
	STRUCTURE_LIMITS	*psStructLimits;

	if (!stackPopParams(2, ST_STRUCTURESTAT, &structInc, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrSetStructureLimits: failed to pop");
		return false;}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "scrSetStructureLimits:player number is too high");
		return false;}

	if (structInc > numStructureStats)
	{
		debug(LOG_ERROR, "scrSetStructureLimits: Structure stat is too high");
		return false;}

	psStructLimits = asStructLimits[player];
	numStructures = (SDWORD)psStructLimits[structInc].currentQuantity;

	scrFunctionResult.v.ival = numStructures;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Return player's unit limit
bool scrGetUnitLimit(void)
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
bool scrMin(void)
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
bool scrMax(void)
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

bool scrFMin(void)
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
bool scrFMax(void)
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

bool ThreatInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs)
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
		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->visible[player])		//can see this droid?
			{
				if (!objHasWeapon((BASE_OBJECT *)psDroid))
				{
					continue;
				}

				//if VTOLs are excluded, skip them
				if (!bVTOLs && ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT) || (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)))
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

//find unrevealed tile closest to pwLooker within the range of wRange
bool scrFogTileInRange(void)
{
	SDWORD		pwLookerX,pwLookerY,tBestX,tBestY,threadRange;
	SDWORD		wRangeX,wRangeY,tRangeX,tRangeY,wRange,player;
	UDWORD		tx,ty,i,j,wDist,wBestDist;
	MAPTILE		*psTile;
	bool		ok = false;
	SDWORD		*wTileX,*wTileY;

	if (!stackPopParams(9, VAL_REF|VAL_INT, &wTileX, VAL_REF|VAL_INT, &wTileY,
		VAL_INT, &pwLookerX, VAL_INT, &pwLookerY, VAL_INT, &wRangeX, VAL_INT, &wRangeY,
		VAL_INT, &wRange, VAL_INT, &player, VAL_INT, &threadRange))
	{
		debug(LOG_ERROR, "scrFogTileInRange: failed to pop");
		return false;}

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
						if(zonedPAT(i,j))	//Can reach this tile
						{
							if((threadRange <= 0) || (!ThreatInRange(player, threadRange, world_coord(i), world_coord(j), false)))
							{
									wBestDist = wDist;
									tBestX = i;
									tBestY = j;
									ok = true;
							}
						}
					}
				}
		  	}
		}
	}

	if(ok)	//something found
	{
		*wTileX = world_coord(tBestX);
		*wTileY = world_coord(tBestY);

		scrFunctionResult.v.bval = true;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			debug(LOG_ERROR, "scrFogTileInRange: stackPushResult failed (found)");
			return false;
		}
	}
	else
	{
		scrFunctionResult.v.bval = false;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			debug(LOG_ERROR, "scrFogTileInRange: stackPushResult failed (not found)");
			return false;
		}
	}

	return true;
}

bool scrMapRevealedInRange(void)
{
	SDWORD		wRangeX,wRangeY,tRangeX,tRangeY,wRange,tRange,player;
	int             i, j;

	if (!stackPopParams(4, VAL_INT, &wRangeX, VAL_INT, &wRangeY,
		VAL_INT, &wRange, VAL_INT, &player))
	{
		debug(LOG_ERROR,  "scrMapRevealedInRange: failed to pop");
		return false;
	}

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
					scrFunctionResult.v.bval = true;
					if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
					{
						return false;
					}

					return true;
		  		}
			}
		}
	}

	//nothing found
	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Returns true if a certain map tile was revealed, ie fog of war was removed */
bool scrMapTileVisible(void)
{
	SDWORD		tileX,tileY,player;

	if (!stackPopParams(3, VAL_INT, &tileX, VAL_INT, &tileY, VAL_INT, &player))
	{
		debug(LOG_ERROR,  "scrMapTileVisible: failed to pop");
		return false;
	}

    //Check coords
	if (tileX < 0
	 || tileX > world_coord(mapWidth)
	 || tileY < 0
	 || tileY > world_coord(mapHeight))
	{
		debug(LOG_ERROR,  "scrMapTileVisible: coords off map");
		return false;
	}

	if(TEST_TILE_VISIBLE( player,mapTile(tileX,tileY) ))
	{
		scrFunctionResult.v.bval = true;

		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			return false;
		}
	}
	else
	{
		//not visible
		scrFunctionResult.v.bval = false;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			return false;
		}
	}

	return true;
}

//return number of reserach topics that are left to be researched
//for a certain technology to become available
bool scrNumResearchLeft(void)
{
	RESEARCH			*psResearch;
	SDWORD				player,iResult;
	UWORD				cur,index,tempIndex;
	SWORD				top;

	UWORD				Stack[400];

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

	index = psResearch->index;
	if (index >= asResearch.size())
	{
		ASSERT( false, "scrNumResearchLeft(): invalid research index" );
		return false;
	}

	if(beingResearchedByAlly(index, player))
	{
		iResult = 1;
	}
	else if(IsResearchCompleted(&asPlayerResList[player][index]))
	{
		iResult = 0;
	}
	else if(IsResearchStarted(&asPlayerResList[player][index]))
	{
		iResult = 1;
	}
	else if(IsResearchPossible(&asPlayerResList[player][index]) || IsResearchCancelled(&asPlayerResList[player][index]))
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
		while(true)			//do
		{
			if (cur >= asResearch[index].pPRList.size())		//this one has no PRs or end of PRs reached
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
				iResult += asResearch[index].pPRList.size();		//add num of PRs this topic has

				tempIndex = asResearch[index].pPRList[cur];		//get cur node's index

				//decide if has to check its PRs
				if(!IsResearchCompleted(&asPlayerResList[player][tempIndex]) &&	//don't touch if completed already
					!skTopicAvail(index,player) &&					//has no unresearched PRs left if available
					!beingResearchedByAlly(index, player))			//will become available soon anyway
				{
					if(asResearch[tempIndex].pPRList.size() > 0)	//node has any nodes itself
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
			if((cur >= asResearch[index].pPRList.size()) && (top <= (-1)))	//nothing left
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
bool beingResearchedByAlly(SDWORD resIndex, SDWORD player)
{
	SDWORD	i;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(i != player && aiCheckAlliances(player,i))
		{
			//check each research facility to see if they are doing this topic.
			if (IsResearchStartedPending(&asPlayerResList[i][resIndex]))
			{
				return true;
			}
		}
	}

	return false;
}

// true if player has completed this research
bool scrResearchCompleted(void)
{
	RESEARCH			*psResearch;
	SDWORD				player;
	UWORD				index;

	if (!stackPopParams(2,ST_RESEARCH, &psResearch, VAL_INT, &player ))
	{
		debug(LOG_ERROR,   "scrResearchCompleted: stack failed");
		return false;
	}

	if(psResearch == NULL)
	{
		debug( LOG_ERROR, "scrResearchCompleted: no such research topic" );
		return false;
	}

	index = psResearch->index;
	if (index >= asResearch.size())
	{
		debug( LOG_ERROR, "scrResearchCompleted: invalid research index" );
		return false;
	}

	if(IsResearchCompleted(&asPlayerResList[player][index]))
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

// true if player has already started researching it
bool scrResearchStarted(void)
{
	RESEARCH			*psResearch;
	SDWORD				player;
	UWORD				index;

	if (!stackPopParams(2,ST_RESEARCH, &psResearch, VAL_INT, &player ))
	{
		debug(LOG_ERROR,  "scrResearchStarted(): stack failed");
		return false;
	}

	if(psResearch == NULL)
	{
		ASSERT( false, ": no such research topic" );
		return false;
	}

	index = psResearch->index;
	if (index >= asResearch.size())
	{
		ASSERT( false, "scrResearchCompleted: invalid research index" );
		return false;
	}

	if (IsResearchStartedPending(&asPlayerResList[player][index]))
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

//returns true if location is dangerous
bool scrThreatInRange(void)
{
	SDWORD		player, range, rangeX, rangeY;
	int32_t		bVTOLs;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(5, VAL_INT, &player, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs))
	{
		debug(LOG_ERROR,  "scrThreatInRange(): stack failed");
		return false;
	}

	scrFunctionResult.v.bval = ThreatInRange(player, range, rangeX, rangeY, bVTOLs);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


bool scrNumEnemyWeapObjInRange(void)
{
	SDWORD		lookingPlayer, range, rangeX, rangeY, i;
	UDWORD		numEnemies = 0;
	int32_t		bVTOLs, bFinished;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(6, VAL_INT, &lookingPlayer, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR,  "scrNumEnemyWeapObjInRange(): stack failed");
		return false;
	}


	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[lookingPlayer][i] == ALLIANCE_FORMED) || (i == lookingPlayer))	//skip allies and myself
		{
			continue;
		}

		numEnemies += numPlayerWeapDroidsInRange(i, lookingPlayer, range, rangeX, rangeY, bVTOLs);
		numEnemies += numPlayerWeapStructsInRange(i, lookingPlayer, range, rangeX, rangeY, bFinished);
	}

	scrFunctionResult.v.ival = numEnemies;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumEnemyWeapObjInRange(): failed to push result");
		return false;
	}

	return true;
}

/* Calculates the total cost of enemy weapon objects in a certain area */
bool scrEnemyWeapObjCostInRange(void)
{
	SDWORD		lookingPlayer, range, rangeX, rangeY, i;
	UDWORD		enemyCost = 0;
	int32_t		bVTOLs, bFinished; // was BOOL (int) ** see warning about conversion

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
bool scrFriendlyWeapObjCostInRange(void)
{
	SDWORD		player, range, rangeX, rangeY, i;
	UDWORD		friendlyCost = 0;
	int32_t		bVTOLs, bFinished;		// was BOOL (int) ** see warning about conversion

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
								   SDWORD rangeX, SDWORD rangeY, bool bVTOLs, bool justCount)
{
	UDWORD	droidCost = 0;

	//check droids
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->visible[lookingPlayer])		//can see this droid?
		{
			if (!objHasWeapon((BASE_OBJECT *)psDroid))
			{
				continue;
			}

			//if VTOLs are excluded, skip them
			if (!bVTOLs && ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT) || (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)))
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

UDWORD numPlayerWeapDroidsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, bool bVTOLs)
{
	return costOrAmountInRange(player, lookingPlayer, range, rangeX, rangeY, bVTOLs, true /*only count*/);
}

UDWORD playerWeapDroidsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range,
								   SDWORD rangeX, SDWORD rangeY, bool bVTOLs)
{
	return costOrAmountInRange(player, lookingPlayer, range, rangeX, rangeY, bVTOLs, false /*total cost*/);
}



UDWORD numPlayerWeapStructsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range,
								   SDWORD rangeX, SDWORD rangeY, bool bFinished)
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
								   SDWORD rangeX, SDWORD rangeY, bool bFinished)
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

bool scrNumEnemyWeapDroidsInRange(void)
{
	SDWORD				lookingPlayer,range,rangeX,rangeY,i;
	UDWORD				numEnemies = 0;
	int32_t				bVTOLs;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(5, VAL_INT, &lookingPlayer, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs))
	{
		debug(LOG_ERROR,  "scrNumEnemyWeapDroidsInRange(): stack failed");
		return false;
	}

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[lookingPlayer][i] == ALLIANCE_FORMED) || (i == lookingPlayer))	//skip allies and myself
		{
			continue;
		}

		numEnemies += numPlayerWeapDroidsInRange(i, lookingPlayer, range, rangeX, rangeY, bVTOLs);
	}

	scrFunctionResult.v.ival = numEnemies;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR,  "scrNumEnemyWeapDroidsInRange(): failed to push result");
		return false;
	}

	return true;
}



bool scrNumEnemyWeapStructsInRange(void)
{
	SDWORD			lookingPlayer, range, rangeX, rangeY, i;
	UDWORD			numEnemies = 0;
	int32_t			bFinished;	// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(5, VAL_INT, &lookingPlayer, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR,  "scrNumEnemyWeapStructsInRange(): stack failed");
		return false;
	}

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[lookingPlayer][i] == ALLIANCE_FORMED) || (i == lookingPlayer))	//skip allies and myself
		{
			continue;
		}

		numEnemies += numPlayerWeapStructsInRange(i, lookingPlayer, range, rangeX, rangeY, bFinished);
	}

	scrFunctionResult.v.ival = numEnemies;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumEnemyWeapStructsInRange(): failed to push result");
		return false;
	}

	return true;
}

bool scrNumFriendlyWeapObjInRange(void)
{
	SDWORD				player, range, rangeX, rangeY, i;
	UDWORD				numFriends = 0;
	int32_t				bVTOLs, bFinished;	// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(6, VAL_INT, &player, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR,  "scrNumFriendlyWeapObjInRange(): stack failed");
		return false;
	}

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[player][i] == ALLIANCE_FORMED) || (i == player))	//skip enemies
		{
			numFriends += numPlayerWeapDroidsInRange(i, player, range, rangeX, rangeY, bVTOLs);
			numFriends += numPlayerWeapStructsInRange(i, player, range, rangeX, rangeY,bFinished);
		}
	}

	scrFunctionResult.v.ival = numFriends;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

bool scrNumFriendlyWeapDroidsInRange(void)
{
	SDWORD		lookingPlayer, range, rangeX, rangeY, i;
	UDWORD		numEnemies = 0;
	int32_t		bVTOLs;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(5, VAL_INT, &lookingPlayer, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs))
	{
		debug(LOG_ERROR,  "scrNumFriendlyWeapDroidsInRange(): stack failed");
		return false;
	}

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if((alliances[lookingPlayer][i] == ALLIANCE_FORMED) || (i == lookingPlayer))
		{
			numEnemies += numPlayerWeapDroidsInRange(i, lookingPlayer, range, rangeX, rangeY, bVTOLs);
		}
	}

	//numEnemies = numEnemyWeapObjInRange(player, range, rangeX, rangeY, bVTOLs);
	scrFunctionResult.v.ival = numEnemies;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumFriendlyWeapDroidsInRange(): failed to push result");
		return false;
	}

	return true;
}



bool scrNumFriendlyWeapStructsInRange(void)
{
	SDWORD				lookingPlayer, range, rangeX, rangeY, i;
	UDWORD				numEnemies = 0;
	int32_t				bFinished;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(5, VAL_INT, &lookingPlayer, VAL_INT, &rangeX,
		VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR, "scrNumFriendlyWeapStructsInRange(): stack failed");
		return false;
	}

	for(i=0; i<MAX_PLAYERS; i++)
	{
		if((alliances[lookingPlayer][i] == ALLIANCE_FORMED) || (i == lookingPlayer))	//skip enemies
		{
			numEnemies += numPlayerWeapStructsInRange(i, lookingPlayer, range, rangeX, rangeY, bFinished);
		}
	}

	scrFunctionResult.v.ival = numEnemies;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR,"scrNumFriendlyWeapStructsInRange(): failed to push result");
		return false;
	}

	return true;
}

bool scrNumPlayerWeapDroidsInRange(void)
{
	SDWORD		targetPlayer, lookingPlayer, range, rangeX, rangeY;
	int32_t		bVTOLs;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(6, VAL_INT, &targetPlayer, VAL_INT, &lookingPlayer,
		VAL_INT, &rangeX, VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bVTOLs))
	{
		debug(LOG_ERROR,"scrNumPlayerWeapDroidsInRange(): stack failed");
		return false;
	}

	scrFunctionResult.v.ival = numPlayerWeapDroidsInRange(targetPlayer, lookingPlayer, range, rangeX, rangeY, bVTOLs);

	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumPlayerWeapDroidsInRange(): failed to push result");
		return false;
	}

	return true;
}

bool scrNumPlayerWeapStructsInRange(void)
{
	SDWORD		targetPlayer, lookingPlayer, range, rangeX, rangeY;
	int32_t		bFinished;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(6, VAL_INT, &targetPlayer, VAL_INT, &lookingPlayer,
		VAL_INT, &rangeX, VAL_INT, &rangeY, VAL_INT, &range, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR,"scrNumPlayerWeapStructsInRange(): stack failed");
		return false;
	}

	scrFunctionResult.v.ival = numPlayerWeapStructsInRange(targetPlayer, lookingPlayer, range, rangeX, rangeY, bFinished);

	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumPlayerWeapStructsInRange(): failed to push result");
		return false;
	}

	return true;
}

bool scrNumPlayerWeapObjInRange(void)
{
	SDWORD				targetPlayer, lookingPlayer, range, rangeX, rangeY;
	UDWORD				numEnemies = 0;
	int32_t				bVTOLs, bFinished;		// was BOOL (int) ** see warning about conversion

	if (!stackPopParams(7, VAL_INT, &targetPlayer, VAL_INT, &lookingPlayer,
						VAL_INT, &rangeX, VAL_INT, &rangeY, VAL_INT, &range,
						VAL_BOOL, &bVTOLs, VAL_BOOL, &bFinished))
	{
		debug(LOG_ERROR,"scrNumPlayerWeapObjInRange(): stack failed");
		return false;
	}

	numEnemies += numPlayerWeapDroidsInRange(targetPlayer, lookingPlayer, range, rangeX, rangeY, bVTOLs);
	numEnemies += numPlayerWeapStructsInRange(targetPlayer, lookingPlayer, range, rangeX, rangeY, bFinished);

	scrFunctionResult.v.ival = numEnemies;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrNumPlayerWeapObjInRange(): failed to push result");
		return false;
	}

	return true;
}

bool scrNumEnemyObjInRange(void)
{
	SDWORD				lookingPlayer, range, rangeX, rangeY;
	int32_t				bVTOLs, bFinished;		// was BOOL (int) ** see warning about conversion

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
						  bool bVTOLs, bool bFinished)
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
				  || psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER))
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
bool scrNumStructsByStatInRange(void)
{
	SDWORD		player, lookingPlayer, index, x, y, range;
	SDWORD		rangeSquared,NumStruct;
	STRUCTURE	*psCurr;
	SDWORD		xdiff, ydiff;
	STRUCTURE_STATS *psTarget;

	if (!stackPopParams(6, ST_STRUCTURESTAT, &index, VAL_INT, &x, VAL_INT, &y,
		VAL_INT, &range, VAL_INT, &lookingPlayer, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrNumStructsByStatInRange(): stack failed");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumStructsByStatInRange:player number is too high" );
		return false;
	}

	if (x < 0
	 || map_coord(x) > (SDWORD)mapWidth)
	{
		ASSERT( false, "scrNumStructsByStatInRange : invalid X coord" );
		return false;
	}
	if (y < 0
	 || map_coord(y) > (SDWORD)mapHeight)
	{
		ASSERT( false,"scrNumStructsByStatInRange : invalid Y coord" );
		return false;
	}
	if (index < (SDWORD)0 || index > (SDWORD)numStructureStats)
	{
		ASSERT( false, "scrNumStructsByStatInRange : Invalid structure stat" );
		return false;
	}
	if (range < (SDWORD)0)
	{
		ASSERT( false, "scrNumStructsByStatInRange : Rnage is less than zero" );
		return false;
	}

	NumStruct = 0;

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
				if(psCurr->visible[lookingPlayer])		//can we see it?
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

bool scrNumStructsByStatInArea(void)
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

bool scrNumStructsByTypeInRange(void)
{
	SDWORD		targetPlayer, lookingPlayer, type, x, y, range;
	SDWORD		rangeSquared,NumStruct;
	STRUCTURE	*psCurr;
	SDWORD		xdiff, ydiff;

	if (!stackPopParams(6, VAL_INT, &lookingPlayer, VAL_INT, &targetPlayer,
		VAL_INT, &type, VAL_INT, &x, VAL_INT, &y, VAL_INT, &range))
	{
		debug(LOG_ERROR,"scrNumStructsByTypeInRange: failed to pop");
		return false;
	}

	if (lookingPlayer >= MAX_PLAYERS || targetPlayer >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumStructsByTypeInRange:player number is too high" );
		return false;
	}

	if (x < 0
	 || map_coord(x) > (SDWORD)mapWidth)
	{
		ASSERT( false, "scrNumStructsByTypeInRange : invalid X coord" );
		return false;
	}

	if (y < 0
	 || map_coord(y) > (SDWORD)mapHeight)
	{
		ASSERT( false,"scrNumStructsByTypeInRange : invalid Y coord" );
		return false;
	}

	if (range < (SDWORD)0)
	{
		ASSERT( false, "scrNumStructsByTypeInRange : Rnage is less than zero" );
		return false;
	}

	NumStruct = 0;

	//now look through the players list of structures to see if this type
	//exists within range
	rangeSquared = range * range;
	for(psCurr = apsStructLists[targetPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		xdiff = (SDWORD)psCurr->pos.x - x;
		ydiff = (SDWORD)psCurr->pos.y - y;
		if (xdiff*xdiff + ydiff*ydiff <= rangeSquared)
		{
			if((type < 0) ||(psCurr->pStructureType->type == type))
			{
				if(psCurr->visible[lookingPlayer])		//can we see it?
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

bool scrNumFeatByTypeInRange(void)
{
	SDWORD		lookingPlayer, type, x, y, range;
	SDWORD		rangeSquared,NumFeat;
	FEATURE		*psCurr;
	SDWORD		xdiff, ydiff;

	if (!stackPopParams(5, VAL_INT, &lookingPlayer,
		VAL_INT, &type, VAL_INT, &x, VAL_INT, &y, VAL_INT, &range))
	{
		debug(LOG_ERROR, "scrNumFeatByTypeInRange(): failed to pop");
		return false;
	}

	if (lookingPlayer >= MAX_PLAYERS)
	{
		ASSERT( false, "scrNumFeatByTypeInRange:player number is too high" );
		return false;
	}

	if (x < 0
	 || map_coord(x) > (SDWORD)mapWidth)
	{
		ASSERT( false, "scrNumFeatByTypeInRange : invalid X coord" );
		return false;
	}

	if (y < 0
	 || map_coord(y) > (SDWORD)mapHeight)
	{
		ASSERT( false,"scrNumFeatByTypeInRange : invalid Y coord" );
		return false;
	}

	if (range < (SDWORD)0)
	{
		ASSERT( false, "scrNumFeatByTypeInRange : Rnage is less than zero" );
		return false;
	}

	NumFeat = 0;

	//now look through the players list of structures to see if this type
	//exists within range
	rangeSquared = range * range;
	for(psCurr = apsFeatureLists[0]; psCurr; psCurr = psCurr->psNext)
	{
		xdiff = (SDWORD)psCurr->pos.x - x;
		ydiff = (SDWORD)psCurr->pos.y - y;
		if (xdiff*xdiff + ydiff*ydiff <= rangeSquared)
		{
			if((type < 0) ||(psCurr->psStats->subType == type))	//like FEAT_OIL_RESOURCE
			{
				if(psCurr->visible[lookingPlayer])		//can we see it?
				{
					NumFeat++;
				}
			}
		}
	}

	scrFunctionResult.v.ival = NumFeat;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

//returns num of visible structures of a certain player in range (only visible ones)
bool scrNumStructsButNotWallsInRangeVis(void)
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

// Only returns structure if it is visible
bool scrGetStructureVis(void)
{
	SDWORD				player, lookingPlayer, index;
	STRUCTURE			*psStruct;
	UDWORD				structType;
	bool				found;

	if (!stackPopParams(3, ST_STRUCTURESTAT, &index, VAL_INT, &player, VAL_INT, &lookingPlayer))
	{
		debug(LOG_ERROR,"scrGetStructureVis: failed to pop");
		return false;
	}

	if ((player >= MAX_PLAYERS) || (lookingPlayer >= MAX_PLAYERS))
	{
		ASSERT( false, "scrGetStructureVis:player number is too high" );
		return false;
	}

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
		psStruct = NULL;
	}

	scrFunctionResult.v.oval = psStruct;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

//returns num of visible structures of a certain player in range
bool scrChooseValidLoc(void)
{
	SDWORD sendY, sendX, *x, *y, player, threatRange;
	UDWORD tx,ty;

	if (!stackPopParams(6, VAL_REF|VAL_INT, &x, VAL_REF|VAL_INT, &y,
		VAL_INT, &sendX, VAL_INT, &sendY, VAL_INT, &player, VAL_INT, &threatRange))
	{
		debug(LOG_ERROR,"scrChooseValidLoc: failed to pop");
		return false;
	}

    //Check coords
	if (sendX < 0
	 || sendX > world_coord(mapWidth)
	 || sendY < 0
	 || sendY > world_coord(mapHeight))
	{
		debug(LOG_ERROR, "scrChooseValidLoc: coords off map");
		return false;
	}

	tx = map_coord(sendX);
	ty = map_coord(sendY);

	if(pickATileGenThreat(&tx, &ty, LOOK_FOR_EMPTY_TILE, threatRange, player, zonedPAT))
	{
		*x = world_coord(tx);
		*y = world_coord(ty);
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

//returns closest enemy object
bool scrGetClosestEnemy(void)
{
	SDWORD				x, y, tx, ty, player, range, i;
	UDWORD				dist, bestDist;
	int32_t				weaponOnly, bVTOLs;		// was BOOL (int) ** see warning about conversion
	bool				bFound = false;			//only military objects?
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

	for (i=0;i<MAX_PLAYERS;i++)
	{
		if ((alliances[player][i] == ALLIANCE_FORMED) || (i == player))
		{
			continue;
		}


		//check droids
		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->visible[player])		//can see this droid?
			{
				//if only weapon droids and don't have it, then skip
				if (weaponOnly && !objHasWeapon((BASE_OBJECT *)psDroid))
				{
					continue;
				}

				//if VTOLs are excluded, skip them
				if (!bVTOLs && ((asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].propulsionType == PROPULSION_TYPE_LIFT) || (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)))
				{
					continue;
				}

				dist = world_coord(hypotf(tx - map_coord(psDroid->pos.x), ty - map_coord(psDroid->pos.y)));
				if (dist < bestDist)
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
		for (psStruct = apsStructLists[i]; psStruct; psStruct=psStruct->psNext)
		{
			if (psStruct->visible[player])	//if can see it
			{
				//only need defenses?
				if (weaponOnly && (!objHasWeapon((BASE_OBJECT *) psStruct) || (psStruct->status != SS_BUILT) ))	//non-weapon-structures	or not finished
				{
					continue;
				}

				dist = world_coord(hypotf(tx - map_coord(psStruct->pos.x), ty - map_coord(psStruct->pos.y)));
				if (dist < bestDist)
				{
					if ((range < 0) || (dist < range))	//in range
					{
						bestDist = dist;
						bFound = true;
						psObj = (BASE_OBJECT*)psStruct;
					}
				}
			}
		}

	}

	if (bFound)
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
bool scrTransporterCapacity(void)
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

	if(psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)
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
bool scrTransporterFlying(void)
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

	if(psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)
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

bool scrUnloadTransporter(void)
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

	if(psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER)
	{
		debug(LOG_ERROR,"scrUnloadTransporter(): passed droid is not a transporter");
		return false;
	}

	unloadTransporter(psDroid,x,y, false);

	return true;
}

//return true if droid is a member of any group
bool scrHasGroup(void)
{
	DROID			*psDroid;
	bool			retval;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		debug( LOG_ERROR,"scrHasGroup: failed to pop" );
		return false;
	}

	if (psDroid == NULL)
	{
		debug( LOG_ERROR, "scrHasGroup: droid is NULLOBJECT" );
		return false;
	}

	if (psDroid->psGroup != NULL)
	{
		retval = true;
	}
	else
	{
		retval = false;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Range is in world units! */
bool scrObjWeaponMaxRange(void)
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

bool scrObjHasWeapon(void)
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

bool scrObjectHasIndirectWeapon(void)
{
	WEAPON_STATS	*psWeapStats;
	bool			bIndirect;
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
bool scrGetClosestEnemyDroidByType(void)
{
	SDWORD				x,y,tx,ty, player, range,i,type;
	UDWORD				dist,bestDist;
	bool				bFound = false;	//only military objects?
	int32_t				bVTOLs;			// was BOOL (int) ** see warning about conversion
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

//returns closest structure by type
bool scrGetClosestEnemyStructByType(void)
{
	SDWORD				x,y,tx,ty, player, range,i,type,dist;
	UDWORD				bestDist;
	bool				bFound = false;	//only military objects?
	STRUCTURE			*psStruct = NULL, *foundStruct = NULL;

	if (!stackPopParams(5, VAL_INT, &x, VAL_INT, &y,
		 VAL_INT, &range,  VAL_INT, &type, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrGetClosestEnemyStructByType: stack failed");
		return false;
	}

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

	if(bFound)
	{
		scrFunctionResult.v.oval = foundStruct;
		if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
		{
			return false;
		}
	}
	else
	{
		scrFunctionResult.v.oval = NULL;
		if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
		{
			return false;
		}
	}

	return true;
}



//Approx point of intersection of a circle and a line with start loc being circle's center point
bool scrCirclePerimPoint(void)
{
	SDWORD				basex,basey,*grx,*gry,radius;
	float factor, deltaX, deltaY;

	if (!stackPopParams(5, VAL_INT, &basex, VAL_INT, &basey, VAL_REF|VAL_INT, &grx,
		VAL_REF|VAL_INT, &gry, VAL_INT, &radius))
	{
		debug(LOG_ERROR,"scrCirclePerimPoint(): stack failed");
		return false;
	}

	if(radius == 0)
	{
		debug(LOG_ERROR,"scrCirclePerimPoint: radius == 0.");
		return true;
	}

	deltaX = (float)(*grx - basex);	//x len (signed!)
	deltaY = (float)(*gry - basey);

	factor =  hypotf(deltaX, deltaY) / (float)radius;			//by what factor is distance > radius?

	//if point was inside of the circle, don't modify passed parameter
	if(factor == 0)
	{
		debug_console("scrCirclePerimPoint: division by zero.");
		return true;
	}

	//calc new len
	deltaX = deltaX / factor;
	deltaY = deltaY / factor;

	//now add new len to the center coords
	*grx = basex + deltaX;
	*gry = basey + deltaY;

	return true;
}

//send my vision to AI
bool scrGiftRadar(void)
{
	SDWORD	playerFrom, playerTo;
	int32_t	playMsg;		// was BOOL (int) ** see warning about conversion

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

bool scrNumAllies(void)
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
bool scrNumAAinRange(void)
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
bool scrSelectDroid(void)
{
	int32_t	bSelect;	// was BOOL (int) ** see warning about conversion
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
bool scrSelectGroup(void)
{
	int32_t		bSelect;	// was BOOL (int) ** see warning about conversion
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

bool scrModulo(void)
{
	SDWORD				num1,num2;

	if (!stackPopParams(2, VAL_INT, &num1, VAL_INT, &num2))
	{
		debug(LOG_ERROR,"scrModulo(): stack failed");
		return false;
	}

	scrFunctionResult.v.ival =  (num1 % num2);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR,"scrModulo(): failed to push result");
		return false;
	}

	return true;
}

bool scrPlayerLoaded(void)
{
	SDWORD			player;
	bool			bPlayerHasFactories=false;
	STRUCTURE		*psCurr;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrPlayerLoaded(): stack failed");
		return false;
	}

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
	scrFunctionResult.v.bval = (apsDroidLists[player] != NULL || bPlayerHasFactories);

	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR,"scrPlayerLoaded(): failed to push result");
		return false;
	}

	return true;
}

/* Add a beacon (blip) */
bool addBeaconBlip(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, const char *textMsg)
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
		debug(LOG_WARNING, "call failed");
	}

	//Received a blip message from a player callback
	//store and call later
	//-------------------------------------------------
	//call beacon callback only if not adding for ourselves
	if(forPlayer != sender)
	{
		if(!msgStackPush(CALL_BEACON,sender,forPlayer,textMsg,locX,locY,NULL))
		{
			debug(LOG_ERROR, "msgStackPush - stack failed");
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

bool sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, const char *beaconMsg)
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
	psViewData = new VIEWDATA;

	//store name
	sprintf(name, _("Beacon %d"), sender);
 	psViewData->pName = name;

	//store text message, hardcoded for now
	psViewData->textMsg.push_back(QString::fromUtf8(getPlayerName(sender)));

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
bool scrDropBeacon(void)
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
bool scrRemoveBeacon(void)
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

bool scrClosestDamagedGroupDroid(void)
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

		if (psDroid->order.psObj != NULL && psDroid->order.psObj->type == OBJ_DROID)
		{
			if ((DROID *)psDroid->order.psObj == psDroidToCheck)
			{
				numRepaired++;
			}
		}
	}

	return numRepaired;
}

/* Uses debug_console() for console debug output right now */
bool scrMsgBox(void)
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		debug(LOG_ERROR, "scrMsgBox(): stack failed");
		return false;
	}

	debug_console("DEBUG: %s",strParam1);

	return true;
}


// Check for a struct being within a certain range of a position (must be visible)
bool scrStructInRangeVis(void)
{
	SDWORD		range, player,lookingPlayer, x,y;
	bool		found;

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
bool scrDroidInRangeVis(void)
{
	SDWORD		range, player,lookingPlayer, x,y;
	bool		found;

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
bool objectInRangeVis(BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range, SDWORD lookingPlayer)
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

/* Go after a certain research */
bool scrPursueResearch(void)
{
	RESEARCH			*psResearch;
	SDWORD				foundIndex = 0, player, cur, tempIndex, Stack[400];
	UDWORD				index;
	SWORD				top;
	bool				found;
	STRUCTURE			*psBuilding;
	RESEARCH_FACILITY	*psResFacilty;

	if (!stackPopParams(3,ST_STRUCTURE, &psBuilding, VAL_INT, &player, ST_RESEARCH, &psResearch ))
	{
		debug(LOG_ERROR, "scrPursueResearch(): stack failed");
		return false;
	}

	if (psResearch == NULL)
	{
		ASSERT(false, ": no such research topic");
		return false;
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

	index = psResearch->index;
	if (index >= asResearch.size())
	{
		ASSERT(false, "scrPursueResearch: invalid research index");
		return false;
	}

	found = false;

	if (beingResearchedByAlly(index, player))		//an ally is already researching it
	{
		found = false;
	}
	else if (IsResearchCompleted(&asPlayerResList[player][index]))
	{
		found = false;
		//DbgMsg("Research already completed: %d", index);
	}
	else if (IsResearchStartedPending(&asPlayerResList[player][index]))
	{
		found = false;
		//DbgMsg("Research already in progress, %d", index);
	}
	else if (IsResearchPossible(&asPlayerResList[player][index]) || IsResearchCancelled(&asPlayerResList[player][index]))
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
		while(true)	//do
		{
			//DbgMsg("Going on with %d, numPR: %d, %s", index, asResearch[index].pPRList.size(), asResearch[index].pName);

			if (cur >= asResearch[index].pPRList.size())		//node has nodes?
			{
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
				else if (!IsResearchCompleted(&asPlayerResList[player][tempIndex])
				         && !IsResearchStartedPending(&asPlayerResList[player][tempIndex]))  //not avail and not busy with it, can check this PR's PR
				{
					//DbgMsg("node not complete, not started: %d, (cur=%d), %s", tempIndex,cur, asResearch[tempIndex].pName);
					if (!asResearch[tempIndex].pPRList.empty())	//node has any nodes itself
					{
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
			if((cur >= asResearch[index].pPRList.size()) && (top <= (-1)))	//nothing left
			{
				break;
			}

		} // while(true)
	}

	if (found && foundIndex < asResearch.size())
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
	scrFunctionResult.v.bval = found;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}
	intRefreshScreen();

	return true;
}

bool scrGetStructureType(void)
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

/* Get player name from index */
bool scrGetPlayerName(void)
{
	SDWORD	player;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrGetPlayerName(): stack failed");
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGetPlayerName: invalid player number" );
		return false;
	}

	/* Casting away constness because stackPushResult doesn't modify it's
	 * value (i.e. in this case it's not const correct).
	 */
	scrFunctionResult.v.sval = (char*)getPlayerName((UDWORD)player);
	if (!stackPushResult(VAL_STRING, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetPlayerName(): failed to push result");
		return false;
	}

	return true;
}

/* Set player name */
bool scrSetPlayerName(void)
{
	SDWORD	player;

	if (!stackPopParams(2, VAL_INT, &player, VAL_STRING, &strParam1))
	{
		debug(LOG_ERROR, "scrSetPlayerName(): stack failed");
		return false;
	}

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrSetPlayerName: invalid player number" );
		return false;
	}

	scrFunctionResult.v.bval = setPlayerName(player, strParam1);
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrSetPlayerName(): failed to push result");
		return false;
	}

	return true;
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
bool scrGetBit(void)
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
bool scrSetBit(void)
{
	SDWORD				base,position;
	int32_t				bSet;	// was BOOL (int) ** see warning about conversion

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

/* Can we create and break alliances? */
bool scrAlliancesLocked(void)
{
	bool		bResult = true;

	if(bMultiPlayer && (game.alliance == ALLIANCES))
		bResult = false;

	scrFunctionResult.v.bval = bResult;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrAlliancesLocked(): failed to push result");
		return false;
	}

	return true;
}

bool scrASSERT(void)
{
	int32_t			bExpression;	// was BOOL (int) ** see warning about conversion
	SDWORD			player;
	char			sTmp[255];

	if (!stackPopParams(3, VAL_BOOL, &bExpression, VAL_STRING, &strParam1, VAL_INT, &player))
	{
		debug(LOG_ERROR, "scrASSERT(): stack failed");
		return false;
	}

#ifdef DEBUG
	/* Just pass the expression and message from script */
	sprintf(sTmp,"%d) %s",player,strParam1);
	ASSERT(bExpression, "%s", sTmp);
#else
	if(scrDebug[player])
	{
		if(!bExpression)
		{
			sprintf(sTmp,"%d) %s",player,strParam1);
			addConsoleMessage(sTmp,RIGHT_JUSTIFY,player);
		}
	}
#endif

	return true;
}

/* Visualize radius at position */
bool scrShowRangeAtPos(void)
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

bool scrToPow(void)
{
	float		x,y;

	if (!stackPopParams(2, VAL_FLOAT, &x, VAL_FLOAT, &y))
	{
		debug(LOG_ERROR, "scrToPow(): stack failed");
		return false;
	}

	scrFunctionResult.v.fval = (float)pow(x,y);
	if (!stackPushResult(VAL_FLOAT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrToPow(): failed to push result");
		return false;
	}

	return true;
}

/* Exponential function */
bool scrExp(void)
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
bool scrSqrt(void)
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
bool scrLog(void)
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
bool scrDebugMenu(void)
{
	int32_t		menuUp;		// was SDWORD which happens to be int32_t, but is being used as a VAL_BOOL, so : ** see warning about conversion

	if (!stackPopParams(1, VAL_BOOL, &menuUp))
	{
		debug(LOG_ERROR, "scrDebugMenu(): stack failed");
		return false;
	}

	(void)addDebugMenu(menuUp);

	return true;
}

/* Set debug menu output string */
bool scrSetDebugMenuEntry(void)
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

/* Parse chat message and return number of commands that could be extracted */
bool scrProcessChatMsg(void)
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		debug(LOG_ERROR, "scrProcessChatMsg(): stack failed");
		return false;
	}

	debug(LOG_NEVER, "Now preparing to parse '%s'", strParam1);

	if (!chatLoad(strParam1, strlen(strParam1)))
	{
		ASSERT(false, "Couldn't process chat message: %s", strParam1);
		return false;
	}

	scrFunctionResult.v.ival = chat_msg.numCommands;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrProcessChatMsg(): failed to push result");
		return false;
	}

	return true;
}

/* Returns number of command arguments for a certain
 * chat command that could be extracted
 */
bool scrGetNumArgsInCmd(void)
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
bool scrGetChatCmdDescription(void)
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
bool scrGetChatCmdParam(void)
{
	SDWORD			cmdIndex, argIndex;
	void			*pArgument=NULL;
	INTERP_TYPE		argType=VAL_VOID;
	bool			bSuccess=true;		//failure on type mismatch

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
bool scrChatCmdIsPlayerAddressed(void)
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
bool scrSetTileHeight(void)
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
bool scrGetTileStructure(void)
{
	SDWORD		structureX,structureY;

	if (!stackPopParams(2, VAL_INT, &structureX, VAL_INT, &structureY))
	{
		debug(LOG_ERROR, "scrGetTileStructure(): stack failed");
		return false;
	}

	scrFunctionResult.v.oval = getTileStructure(structureX, structureY);
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrGetTileStructure(): failed to push result");
		return false;
	}

	return true;
}

/* Outputs script call stack
 */
bool scrPrintCallStack(void)
{
	scrOutputCallTrace(LOG_SCRIPT);

	return true;
}

/*
 * Returns true if game debug mode is on
 */
bool scrDebugModeEnabled(void)
{

	scrFunctionResult.v.bval = getDebugMappingStatus();
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrDebugModeEnabled(): failed to push result");
		return false;
	}

	return true;
}

/*
 * Returns the cost of a droid
 */
bool scrCalcDroidPower(void)
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
bool scrGetDroidLevel(void)
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

/* Assembles a template from components and returns it */
bool scrAssembleWeaponTemplate(void)
{
	SDWORD					player,bodyIndex,weapIndex,propIndex;
	DROID_TEMPLATE			*pNewTemplate = NULL;

	if (!stackPopParams(4, VAL_INT, &player, ST_BODY, &bodyIndex,
		ST_PROPULSION, &propIndex, ST_WEAPON, &weapIndex))
	{
		return false;
	}

	pNewTemplate = new DROID_TEMPLATE;
	if (pNewTemplate == NULL)
	{
		debug(LOG_ERROR, "pNewTemplate: Out of memory");
		return false;
	}

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
		return false;
	}

	// make sure we have a valid weapon
	if(!checkValidWeaponForProp(pNewTemplate))
	{
		scrFunctionResult.v.oval = NULL;		// failure
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
				sendTemplate(player, pNewTemplate);
			}
		}
		else
		{
			// free resources
			delete pNewTemplate;

			// already exists, so return it
			pNewTemplate = tempTemplate;
		}

		scrFunctionResult.v.oval = pNewTemplate;	// succes
	}

	// return template to scripts
	if (!stackPushResult((INTERP_TYPE)ST_TEMPLATE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Checks if template already exists, returns it if yes */
static DROID_TEMPLATE* scrCheckTemplateExists(SDWORD player, DROID_TEMPLATE *psTempl)
{
	DROID_TEMPLATE* psCurrent;
	bool equal;

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

bool scrWeaponLongHitUpgrade(void)
{
	SDWORD					player,weapIndex;
	const WEAPON_STATS		*psWeapStats;

	if (!stackPopParams(2, VAL_INT, &player, ST_WEAPON, &weapIndex))
	{
		return false;
	}

	psWeapStats = &asWeaponStats[weapIndex];

	scrFunctionResult.v.ival = asWeaponUpgrade[player][psWeapStats->weaponSubClass].longHit;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

bool scrWeaponDamageUpgrade(void)
{
	SDWORD					player,weapIndex;
	const WEAPON_STATS		*psWeapStats;

	if (!stackPopParams(2, VAL_INT, &player, ST_WEAPON, &weapIndex))
	{
		return false;
	}

	psWeapStats = &asWeaponStats[weapIndex];

	scrFunctionResult.v.ival = asWeaponUpgrade[player][psWeapStats->weaponSubClass].damage;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

bool scrWeaponFirePauseUpgrade(void)
{
	SDWORD					player,weapIndex;
	const WEAPON_STATS		*psWeapStats;

	if (!stackPopParams(2, VAL_INT, &player, ST_WEAPON, &weapIndex))
	{
		return false;
	}

	psWeapStats = &asWeaponStats[weapIndex];

	scrFunctionResult.v.ival = asWeaponUpgrade[player][psWeapStats->weaponSubClass].firePause;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


bool scrIsComponentAvailable(void)
{
	SDWORD					player;
	bool					bAvailable = false;
	INTERP_VAL				sVal;

	if (!stackPop(&sVal))
	{
		return false;
	}

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( false, "player number is too high" );
		return false;
	}

	switch ((unsigned)sVal.type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
	{
	case ST_BODY:
		bAvailable = (apCompLists[player][COMP_BODY][sVal.v.ival] == AVAILABLE);
		break;
	case ST_PROPULSION:
		bAvailable = (apCompLists[player][COMP_PROPULSION][sVal.v.ival] == AVAILABLE);
		break;
	case ST_ECM:
		bAvailable = (apCompLists[player][COMP_ECM][sVal.v.ival] == AVAILABLE);
		break;
	case ST_SENSOR:
		bAvailable = (apCompLists[player][COMP_SENSOR][sVal.v.ival] == AVAILABLE);
		break;
	case ST_CONSTRUCT:
		bAvailable = (apCompLists[player][COMP_CONSTRUCT][sVal.v.ival] == AVAILABLE);
		break;
	case ST_WEAPON:
		bAvailable = (apCompLists[player][COMP_WEAPON][sVal.v.ival] == AVAILABLE);
		break;
	case ST_REPAIR:
		bAvailable = (apCompLists[player][COMP_REPAIRUNIT][sVal.v.ival] == AVAILABLE);
		break;
	case ST_BRAIN:
		bAvailable = (apCompLists[player][COMP_BRAIN][sVal.v.ival] == AVAILABLE);
		break;
	default:
		ASSERT( false, "unknown component type" );
		return false;
	}

	scrFunctionResult.v.bval = bAvailable;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

bool scrGetBodySize(void)
{
	SDWORD		bodyIndex;

	if (!stackPopParams(1,ST_BODY, &bodyIndex))
	{
		return false;
	}

	scrFunctionResult.v.ival = asBodyStats[bodyIndex].size;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

bool scrGettext()
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		return false;
	}

	scrFunctionResult.v.sval = (char*)gettext(strParam1);

	return stackPushResult((INTERP_TYPE)ST_TEXTSTRING, &scrFunctionResult);
}

bool scrGettext_noop()
{
	if (!stackPopParams(1, VAL_STRING, &strParam1))
	{
		return false;
	}

	scrFunctionResult.v.sval = gettext_noop(strParam1);

	return stackPushResult(VAL_STRING, &scrFunctionResult);
}

bool scrPgettext()
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

bool scrPgettext_expr()
{
	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_STRING, &strParam2))
	{
		return false;
	}

	scrFunctionResult.v.sval = (char*)pgettext_expr(strParam1, strParam2);

	return stackPushResult((INTERP_TYPE)ST_TEXTSTRING, &scrFunctionResult);
}

bool scrPgettext_noop()
{
	if (!stackPopParams(2, VAL_STRING, &strParam1, VAL_STRING, &strParam2))
	{
		return false;
	}

	scrFunctionResult.v.sval = gettext_noop(strParam1);

	return stackPushResult(VAL_STRING, &scrFunctionResult);
}
