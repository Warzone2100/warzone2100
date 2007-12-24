/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
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
/*
 * Multistruct.c
 *
 * Alex Lee 98, Pumpkin Studios.
 *
 * files to cope with multiplayer structure related stuff..
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "droid.h"
#include "droiddef.h"
#include "basedef.h"
#include "power.h"
#include "geometry.h"								// for gettilestructure
#include "anim_id.h"
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

// ////////////////////////////////////////////////////////////////////////////
// structures

// ////////////////////////////////////////////////////////////////////////////
// INFORM others that a building has been started, and base plate should be put down.
BOOL sendBuildStarted(const STRUCTURE* psStruct, const DROID* psDroid)
{
	NETMSG	msg;
	UDWORD zero=0;
	UWORD player,order;
	player = (UBYTE)psDroid->player;
	order = (UBYTE)psDroid->order;
	NetAdd(msg,0,player);			//player
	NetAdd(msg,1,psDroid->psTarStats->ref);	//id of thing to build
	NetAdd(msg,5,psDroid->orderX);					// x
	NetAdd(msg,7,psDroid->orderY);					// y
	NetAdd(msg,11,psDroid->id);						// droid to order to build it
	NetAdd(msg,15,psStruct->id);					// building id to create
	NetAdd(msg,19,order);			// building id to create

	if (psDroid->psTarget && psDroid->psTarget->type == OBJ_STRUCTURE)
	{
		NetAdd(msg,20,((STRUCTURE*)psDroid->psTarget)->id);
	}
	else
	{
		NetAdd(msg,20,zero);
	}

	NetAdd(msg,24,psStruct->pos.z);

	msg.size =28;
	msg.type = NET_BUILD;
	return (NETbcast(&msg,FALSE));
}

// ////////////////////////////////////////////////////////////////////////////
// put down a base plate and start droid building it!
BOOL recvBuildStarted(NETMSG *pMsg)
{
	UDWORD			targetId,order,droidId,structId,structStat;
	UWORD			x,z,y,player;
	STRUCTURE_STATS *psStats;
	DROID			*psDroid;
	UDWORD			typeindex,actionX,actionY;

	player = pMsg->body[0];					// decode message.
	NetGet(pMsg,1,structStat);
	NetGet(pMsg,5,x);
	NetGet(pMsg,7,y);
	NetGet(pMsg,11,droidId);
	NetGet(pMsg,15,structId);
	order = pMsg->body[19];
	NetGet(pMsg,20,targetId);
	NetGet(pMsg,24,z);
	for(typeindex=0;										// find structure target
	(typeindex<numStructureStats ) && (asStructureStats[typeindex].ref != structStat);
	typeindex++);

	psStats = &asStructureStats[typeindex];

	if(IdToDroid(droidId,player, &psDroid))
	{
		if (getDroidDestination( (BASE_STATS *)psStats ,x,y, &actionX,&actionY))
		{
			psDroid->order = order;
			if(psDroid->order == DORDER_LINEBUILD)
			{
				psDroid->order = DORDER_BUILD;
			}
			psDroid->orderX = x;
			psDroid->orderY = y;
			psDroid->psTarStats = (BASE_STATS *) psStats;
			if(targetId)
			{
				setDroidTarget(psDroid, IdToPointer(targetId, ANYPLAYER));
			}
			else
			{
				setDroidTarget(psDroid, NULL);
			}

			if (IsStatExpansionModule(psStats))
			{
				setUpBuildModule(psDroid);
			}
			else
			{
				droidStartBuild(psDroid);
				psDroid->action = DACTION_BUILD;
			}

		}

		if (psDroid->psTarget)									//sync id's
		{
			((STRUCTURE*)psDroid->psTarget)->id = structId;
		}
	}

	// order droid to start building it.
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// INFORM others that a building has been completed.
BOOL SendBuildFinished(const STRUCTURE* psStruct)
{
	NETMSG m;

	NetAdd(m,0,psStruct->id);							//id of finished struct
	// also enough info to build it if we don't already know about it.
	NetAdd(m,4,psStruct->pStructureType->ref);			// kind of building.
	NetAdd(m,8,psStruct->pos.x);							// x pos
	NetAdd(m,10,psStruct->pos.y);							// y pos
	NetAdd(m,12,psStruct->pos.z);							// y pos
	m.body[14] =(char) psStruct->player;				// player

	m.size =15;
	m.type =NET_BUILDFINISHED;
	return (NETbcast(&m,FALSE));
}

// ////////////////////////////////////////////////////////////////////////////
BOOL recvBuildFinished(NETMSG *m)
{
	UDWORD strId;//,i;
	STRUCTURE *psStr;
	UWORD	x,y,z;
	UDWORD	type,typeindex;
	UBYTE	player;

	NetGet(m,0,strId);									// get the struct id.
	psStr = IdToStruct(strId,ANYPLAYER);

	if(psStr)
	{												// make it complete.
		psStr->currentBuildPts = psStr->pStructureType->buildPoints+1;

		if(psStr->status != SS_BUILT)
		{
			psStr->status = SS_BUILT;
			buildingComplete(psStr);
		}
		NETlogEntry("building finished ok." ,0,0);
		return TRUE;
	}

	// the building wasn't started, so we'll have to just plonk it down in the map.
	NetGet(m,4,type);									// kind of building.
	NetGet(m,8,x);										// x pos
	NetGet(m,10,y);										// y pos
	NetGet(m,12,z);										// z pos

	player = m->body[14];								// player

	for(typeindex=0;														// find structure target
		(typeindex<numStructureStats ) && (asStructureStats[typeindex].ref != type);
		typeindex++);
	psStr = 0;

	// check for similar buildings, to avoid overlaps
	if (TILE_HAS_STRUCTURE(mapTile(map_coord(x), map_coord(y))))
	{
		// get structure;
		psStr = getTileStructure(map_coord(x), map_coord(y));
		if(asStructureStats[typeindex].type == psStr->pStructureType->type)
		{
			// correct type, correct location, just rename the id's to sync it.. (urgh)
			psStr->id = strId;
			psStr->status = SS_BUILT;
			buildingComplete(psStr);
			NETlogEntry("structure id modified" ,0,player);
			return TRUE;
		}
	}

	psStr = buildStructure(&(asStructureStats[typeindex]),					// build the structure.
					x,y,
					player,TRUE);
	if (psStr)
	{
		psStr->id		= strId;
		psStr->status	= SS_BUILT;
		buildingComplete(psStr);

		DBCONPRINTF(ConsoleString,(ConsoleString,"MultiPlayer: Struct not found on recvbuildcomplete :%d",strId ));
		NETlogEntry("had to plonk down a building" ,0,player);
	}
	else
	{
		DBCONPRINTF(ConsoleString,(ConsoleString,"MultiPlayer: Struct not found on recvbuildcomplete BUILDIT FAILED TOO!:%d",strId ));
		NETlogEntry("had to plonk down a building, BUT FAILED OH S**T." ,0,player);
	}
	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////////
// demolish message.
BOOL SendDemolishFinished( STRUCTURE *psStruct,DROID *psDroid)
{
	NETMSG m;

	NetAdd(m,0,psStruct->id);
	NetAdd(m,4,psDroid->id);

	m.size = 2*sizeof(UDWORD);
	m.type = NET_DEMOLISH;
	return( NETbcast(&m,FALSE));
}

BOOL recvDemolishFinished(NETMSG *m)
{
	STRUCTURE	*psStruct;
	UDWORD		s,d;
	DROID		*psDroid;

	NetGet(m,0,s);
	NetGet(m,4,d);

	psStruct = IdToStruct(s,ANYPLAYER);
	IdToDroid(d,ANYPLAYER,&psDroid);

	if(psStruct)
	{
		removeStruct( psStruct, TRUE );				// demolish it.
		if(psDroid && psDroid->psTarStats)
		{
			psDroid->psTarStats = NULL;		// update droid if reqd.
		}
	}
	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////////
// Inform others that a structure has been destroyed
BOOL SendDestroyStructure(const STRUCTURE* s)
{
	NETMSG m;

	technologyGiveAway(s);

	NetAdd(m,0,s->id);									// struct to destroy
	m.size =sizeof(UDWORD);
	m.type=NET_STRUCTDEST;
	return( NETbcast(&m,FALSE));
}

// ////////////////////////////////////////////////////////////////////////////
// acknowledge the destruction of a structure, from another player.
BOOL recvDestroyStructure(NETMSG * m)
{
	UDWORD s;
	STRUCTURE *psStr;

	NetGet(m,0,s);								// struct to destory

	psStr = IdToStruct(s,ANYPLAYER);
	if (psStr)
	{
		turnOffMultiMsg(TRUE);
		destroyStruct(psStr);				// remove the struct from remote players machine.
		turnOffMultiMsg(FALSE);

		technologyGiveAway(psStr);

		return (TRUE);
	}
	return (TRUE);
}


// ////////////////////////////////////////////////////////////////////////////
//lassat is firing

BOOL sendLasSat(UBYTE player, STRUCTURE *psStruct, BASE_OBJECT *psObj)
{
	DBCONPRINTF(ConsoleString,(ConsoleString,"sendLasSat() called"));
	NETbeginEncode(NET_LASSAT, NET_ALL_PLAYERS);

		NETuint8_t(&player);
		NETuint32_t(&psStruct->id);
		NETuint32_t(&psObj->id);	// Target
		NETuint8_t(&psObj->player);	// Target player

	return NETend();
}

// recv lassat info on the receiving end.
BOOL recvLasSat()
{
	BASE_OBJECT	*psObj;
	UBYTE		player,targetplayer;
	STRUCTURE	*psStruct;
	uint32_t	id,targetid;
	DBCONPRINTF(ConsoleString,(ConsoleString,"recvLasSat() called"));
	
	NETbeginDecode();

		NETuint8_t(&player);
		NETuint32_t(&id);
		NETuint32_t(&targetid);
		NETuint8_t(&targetplayer);
	
		psStruct = IdToStruct (id, player);
		psObj	 = IdToPointer(targetid, targetplayer);
	
		if( psStruct && psObj)
		{
			// Give enemy no quarter, unleash the lasat
			proj_SendProjectile(&psStruct->asWeaps[0], NULL, player, psObj->pos.x,
	            psObj->pos.y, psObj->pos.z, psObj, TRUE, FALSE, 0);
	      	// Play 5 second countdown message
			audio_QueueTrackPos( ID_SOUND_LAS_SAT_COUNTDOWN, psObj->pos.x, psObj->pos.y,
	            psObj->pos.z);
		}

	NETend();
	return TRUE;
}
