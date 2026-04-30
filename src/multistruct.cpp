/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 * Multistruct.c
 *
 * Alex Lee 98, Pumpkin Studios.
 *
 * files to cope with multiplayer structure related stuff..
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"

#include "design.h"
#include "template.h"
#include "droid.h"
#include "droiddef.h"
#include "basedef.h"
#include "power.h"
#include "geometry.h"								// for gettilestructure
#include "stats.h"
#include "map.h"
#include "console.h"
#include "action.h"
#include "order.h"
#include "projectile.h"
#include "lib/netplay/netplay.h"								// the netplay library.
#include "multiplay.h"
#include "multigifts.h"
#include "multirecv.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "research.h"
#include "qtscript.h"
#include "combat.h"

// ////////////////////////////////////////////////////////////////////////////
// structures

// ////////////////////////////////////////////////////////////////////////////
// INFORM others that a building has been completed.
bool SendBuildFinished(const STRUCTURE *psStruct)
{
	uint8_t player = psStruct->player;
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "invalid player %u", player);

	auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_DEBUG_ADD_STRUCTURE);
	NETuint32_t(w, psStruct->id);		// ID of building

	// Along with enough info to build it (if needed)
	NETuint32_t(w, psStruct->pStructureType->ref);
	NETPosition(w, psStruct->pos);
	NETuint8_t(w, player);
	return NETend(w);
}

// ////////////////////////////////////////////////////////////////////////////
bool recvBuildFinished(NETQUEUE queue)
{
	uint32_t	structId;
	STRUCTURE	*psStruct;
	Position        pos;
	uint32_t	type, typeindex;
	uint8_t		player;

	auto r = NETbeginDecode(queue, GAME_DEBUG_ADD_STRUCTURE);
	NETuint32_t(r, structId);	// get the struct id.
	NETuint32_t(r, type); 	// Kind of building.
	NETPosition(r, pos);      // pos
	NETuint8_t(r, player);
	NETend(r);

	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "invalid player %u", player);

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!dbgInputManager.debugMappingsAllowed() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to add structure for player %u.", NetPlay.players[queue.index].position);
		return false;
	}

	psStruct = IdToStruct(structId, ANYPLAYER);

	if (psStruct)
	{
		// make it complete.
		psStruct->currentBuildPts = structureBuildPointsToCompletion(*psStruct) + 1;

		if (psStruct->status != SS_BUILT)
		{
			debug(LOG_SYNC, "Synch error, structure %u was not complete, and should have been.", structId);
			psStruct->status = SS_BUILT;
			buildingComplete(psStruct);
		}
		debug(LOG_SYNC, "Created normal building %u for player %u", psStruct->id, player);
		return true;
	}

	// The building wasn't started, so we'll have to just plonk it down in the map.

	// Find the structures stats
	for (typeindex = 0; typeindex < numStructureStats && asStructureStats[typeindex].ref != type; typeindex++) {}	// Find structure target

	// Build the structure
	psStruct = buildStructureDir(&(asStructureStats[typeindex]), pos.x, pos.y, 0, player, true, structId);
	if (psStruct)
	{
		psStruct->status	= SS_BUILT;
		buildingComplete(psStruct);
		debug(LOG_SYNC, "Huge synch error, forced to create building %u for player %u", psStruct->id, player);
#if defined (DEBUG)
		NETlogEntry("had to plonk down a building", SYNC_FLAG, player);
#endif
		triggerEventStructBuilt(psStruct, nullptr);
		checkPlayerBuiltHQ(psStruct);
	}
	else
	{
		debug(LOG_SYNC, "Gigantic synch error, unable to create building for player %u", player);
		NETlogEntry("had to plonk down a building, BUT FAILED!", SYNC_FLAG, player);
	}

	return false;
}


// ////////////////////////////////////////////////////////////////////////////
// Inform others that a structure has been destroyed
bool SendDestroyStructure(const STRUCTURE *s)
{
	auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_DEBUG_REMOVE_STRUCTURE);
	// Struct to destroy
	NETuint32_t(w, s->id);

	return NETend(w);
}

// ////////////////////////////////////////////////////////////////////////////
// acknowledge the destruction of a structure, from another player.
bool recvDestroyStructure(NETQUEUE queue)
{
	uint32_t structID;
	STRUCTURE *psStruct;

	auto r = NETbeginDecode(queue, GAME_DEBUG_REMOVE_STRUCTURE);
	NETuint32_t(r, structID);
	NETend(r);

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!dbgInputManager.debugMappingsAllowed() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to remove structure for player %u.", NetPlay.players[queue.index].position);
		return false;
	}

	// Struct to destroy
	psStruct = IdToStruct(structID, ANYPLAYER);

	if (psStruct)
	{
		turnOffMultiMsg(true);
		// Remove the struct from remote players machine
		destroyStruct(psStruct, gameTime - deltaGameTime + 1);  // deltaGameTime is actually 0 here, since we're between updates. However, the value of gameTime - deltaGameTime + 1 will not change when we start the next tick.
		turnOffMultiMsg(false);
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
//lassat is firing

bool sendLasSat(UBYTE player, const STRUCTURE *psStruct, const BASE_OBJECT *psObj)
{
	auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_LASSAT);
	NETuint8_t(w, player);
	NETuint32_t(w, psStruct->id);
	NETuint32_t(w, psObj->id);	// Target
	NETuint8_t(w, psObj->player);	// Target player

	return NETend(w);
}

// recv lassat info on the receiving end.
bool recvLasSat(NETQUEUE queue)
{
	BASE_OBJECT	*psObj;
	UBYTE		player, targetplayer;
	STRUCTURE	*psStruct;
	uint32_t	id, targetid;

	auto r = NETbeginDecode(queue, GAME_LASSAT);
	NETuint8_t(r, player);
	NETuint32_t(r, id);
	NETuint32_t(r, targetid);
	NETuint8_t(r, targetplayer);
	NETend(r);

	psStruct = IdToStruct(id, player);
	psObj	 = IdToPointer(targetid, targetplayer);
	if (psStruct && !canGiveOrdersFor(queue.index, psStruct->player))
	{
		syncDebug("Wrong player.");
		return false;
	}

	if (psStruct && psObj && isLasSat(psStruct->pStructureType))
	{
		// Lassats have just one weapon
		unsigned firePause = weaponFirePause(*psStruct->getWeaponStats(0), player);
		unsigned damLevel = PERCENT(psStruct->body, psStruct->structureBody());

		if (damLevel < HEAVY_DAMAGE_LEVEL)
		{
			firePause += firePause;
		}

		if (isHumanPlayer(player) && gameTime - psStruct->asWeaps[0].lastFired <= firePause)
		{
			/* Too soon to fire again */
			return true ^ false;  // Return value meaningless and ignored.
		}

		// Give enemy no quarter, unleash the lasat
		proj_SendProjectile(&psStruct->asWeaps[0], nullptr, player, psObj->pos, psObj, true, 0);
		psStruct->asWeaps[0].lastFired = gameTime;
		psStruct->asWeaps[0].ammo = 1; // abducting this field for keeping track of triggers

		// Play 5 second countdown message
		audio_QueueTrackPos(ID_SOUND_LAS_SAT_COUNTDOWN, psObj->pos.x, psObj->pos.y, psObj->pos.z);
	}

	return true;
}

void sendStructureInfo(const STRUCTURE *psStruct, STRUCTURE_INFO structureInfo_, const DROID_TEMPLATE *pT)
{
	uint8_t  player = psStruct->player;
	uint32_t structId = psStruct->id;
	uint8_t  structureInfo = structureInfo_;

	auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_STRUCTUREINFO);
	NETuint8_t(w, player);
	NETuint32_t(w, structId);
	NETuint8_t(w, structureInfo);
	if (structureInfo_ == STRUCTUREINFO_MANUFACTURE)
	{
		int32_t droidType = pT->droidType;
		NETwzstring(w, pT->name);
		NETuint32_t(w, pT->multiPlayerID);
		NETint32_t(w, droidType);
		NETuint8_t(w, pT->asParts[COMP_BODY]);
		NETuint8_t(w, pT->asParts[COMP_BRAIN]);
		NETuint8_t(w, pT->asParts[COMP_PROPULSION]);
		NETuint8_t(w, pT->asParts[COMP_REPAIRUNIT]);
		NETuint8_t(w, pT->asParts[COMP_ECM]);
		NETuint8_t(w, pT->asParts[COMP_SENSOR]);
		NETuint8_t(w, pT->asParts[COMP_CONSTRUCT]);
		NETint8_t(w, pT->numWeaps);
		for (int i = 0; i < pT->numWeaps; i++)
		{
			NETuint32_t(w, pT->asWeaps[i]);
		}
	}
	NETend(w);
}

void recvStructureInfo(NETQUEUE queue)
{
	uint8_t         player = 0;
	uint32_t        structId = 0;
	uint8_t         structureInfo;
	STRUCTURE      *psStruct;
	DROID_TEMPLATE t, *pT = &t;
	int32_t droidType;

	auto r = NETbeginDecode(queue, GAME_STRUCTUREINFO);
	NETuint8_t(r, player);
	NETuint32_t(r, structId);
	NETuint8_t(r, structureInfo);
	if (structureInfo == STRUCTUREINFO_MANUFACTURE)
	{
		WzString name;
		NETwzstring(r, name);
		pT->name = name;
		NETuint32_t(r, pT->multiPlayerID);
		NETint32_t(r, droidType);
		NETuint8_t(r, pT->asParts[COMP_BODY]);
		NETuint8_t(r, pT->asParts[COMP_BRAIN]);
		NETuint8_t(r, pT->asParts[COMP_PROPULSION]);
		NETuint8_t(r, pT->asParts[COMP_REPAIRUNIT]);
		NETuint8_t(r, pT->asParts[COMP_ECM]);
		NETuint8_t(r, pT->asParts[COMP_SENSOR]);
		NETuint8_t(r, pT->asParts[COMP_CONSTRUCT]);
		NETint8_t(r, pT->numWeaps);
		ASSERT_OR_RETURN(, pT->numWeaps >= 0 && pT->numWeaps <= ARRAY_SIZE(pT->asWeaps), "Bad numWeaps %d", pT->numWeaps);
		for (int i = 0; i < pT->numWeaps; i++)
		{
			NETuint32_t(r, pT->asWeaps[i]);
		}
		pT->droidType = (DROID_TYPE)droidType;
		pT = copyTemplate(player, pT);
	}
	NETend(r);

	psStruct = IdToStruct(structId, player);

	syncDebug("player%d,structId%u%c,structureInfo%u", player, structId, psStruct == nullptr? '^' : '*', structureInfo);

	if (psStruct == nullptr)
	{
		debug(LOG_WARNING, "Could not find structure %u to change production for", structId);
		return;
	}
	if (!canGiveOrdersFor(queue.index, psStruct->player))
	{
		syncDebug("Wrong player.");
		return;
	}

	CHECK_STRUCTURE(psStruct);

	if (structureInfo == STRUCTUREINFO_MANUFACTURE && !researchedTemplate(pT, player, true, true))
	{
		debug(LOG_ERROR, "Invalid droid received from player %d with name %s", (int)player, pT->name.toUtf8().c_str());
		return;
	}
	if (structureInfo == STRUCTUREINFO_MANUFACTURE && !intValidTemplate(pT, nullptr, true, player))
	{
		debug(LOG_ERROR, "Illegal droid received from player %d with name %s", (int)player, pT->name.toUtf8().c_str());
		return;
	}

	if (psStruct->isFactory())
	{
		popStatusPending(psStruct->pFunctionality->factory);
	}
	else if (psStruct->pStructureType->type == REF_RESEARCH)
	{
		popStatusPending(psStruct->pFunctionality->researchFacility);
	}

	syncDebugStructure(psStruct, '<');

	switch (structureInfo)
	{
	case STRUCTUREINFO_MANUFACTURE:       structSetManufacture(psStruct, pT, ModeImmediate); break;
	case STRUCTUREINFO_CANCELPRODUCTION:  cancelProduction(psStruct, ModeImmediate, false);       break;
	case STRUCTUREINFO_HOLDPRODUCTION:    holdProduction(psStruct, ModeImmediate);                break;
	case STRUCTUREINFO_RELEASEPRODUCTION: releaseProduction(psStruct, ModeImmediate);             break;
	case STRUCTUREINFO_HOLDRESEARCH:      holdResearch(psStruct, ModeImmediate);                  break;
	case STRUCTUREINFO_RELEASERESEARCH:   releaseResearch(psStruct, ModeImmediate);               break;
	default:
		debug(LOG_ERROR, "Invalid structureInfo %d", structureInfo);
	}

	syncDebugStructure(psStruct, '>');

	CHECK_STRUCTURE(psStruct);
}
