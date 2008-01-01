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
 * ScriptObj.c
 *
 * Object access functions for the script library
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "objects.h"

#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptobj.h"
#include "group.h"
#include "lib/gamelib/gtime.h"
#include "cluster.h"
#include "messagedef.h"
#include "message.h"
#include "researchdef.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "multiplay.h"

#include "text.h"
#include "levels.h"
#include "scriptvals.h"
#include "research.h"

static INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

// Get values from a base object
BOOL scrBaseObjGet(UDWORD index)
{
	INTERP_TYPE		type = 0;
	BASE_OBJECT		*psObj;
	DROID			*psDroid;
	STRUCTURE		*psStruct;
	FEATURE			*psFeature;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrBaseObjGet: stackPopParams failed");
		return FALSE;
	}

	// Check this is a valid pointer
	if (psObj == NULL )
	{
		debug(LOG_ERROR, "scrBaseObjGet: was passed an invalid pointer");
		return FALSE;
	}
	// Check this is a valid pointer
	if (psObj->type != OBJ_DROID && psObj->type != OBJ_STRUCTURE && psObj->type != OBJ_FEATURE)
	{
		debug(LOG_ERROR, "scrBaseObjGet: invalid object");
		return FALSE;
	}

	// set the type and return value
	switch (index)
	{
	case OBJID_POSX:
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)psObj->pos.x;
		break;
	case OBJID_POSY:
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)psObj->pos.y;
		break;
	case OBJID_POSZ:
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)psObj->pos.z;
		break;
	case OBJID_ID:
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)psObj->id;
		break;
	case OBJID_PLAYER:
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)psObj->player;
		break;
	case OBJID_TYPE:
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)psObj->type;
		break;
	case OBJID_ORDER:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: order only valid for a droid");
			return FALSE;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->order;
		if (scrFunctionResult.v.ival == DORDER_GUARD && ((DROID *)psObj)->psTarget == NULL)
		{
			scrFunctionResult.v.ival = DORDER_NONE;
		}
		break;
	//new member variable
	case OBJID_ACTION:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: action only valid for a droid");
			return FALSE;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->action;
		break;
	//new member variable - if droid is selected (humans only)
	case OBJID_SELECTED:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: selected only valid for a droid");
			return FALSE;
		}
		type = VAL_BOOL;
		scrFunctionResult.v.bval = (SDWORD)((DROID *)psObj)->selected;
		break;

	case OBJID_STRUCTSTATTYPE:
		if (psObj->type == OBJ_STRUCTURE)
		{
			type = VAL_INT;
			scrFunctionResult.v.ival = ((STRUCTURE *)psObj)->pStructureType->type;
		}
		else
		{
			debug(LOG_ERROR, ".stattype is only supported by Structures");
			return FALSE;
		}
		break;

	case OBJID_ORDERX:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: order only valid for a droid");
			return FALSE;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->orderX;
		break;
	case OBJID_ORDERY:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: order only valid for a droid");
			return FALSE;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->orderY;
		break;
	case OBJID_DROIDTYPE:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: droidType only valid for a droid");
			return FALSE;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->droidType;
		break;
	case OBJID_CLUSTERID:
		if (psObj->type == OBJ_FEATURE)
		{
			debug(LOG_ERROR, "scrBaseObjGet: clusterID not valid for features");
			return FALSE;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = clustGetClusterID(psObj);
		break;
	case OBJID_HEALTH:
		switch (psObj->type)
		{
		case OBJ_DROID:
			psDroid = (DROID *)psObj;
			type = VAL_INT;
			scrFunctionResult.v.ival = psDroid->body * 100 / psDroid->originalBody;
			break;
		case OBJ_FEATURE:
			psFeature = (FEATURE *)psObj;
			type = VAL_INT;
			if (psFeature->psStats->damageable)
			{
				scrFunctionResult.v.ival = psFeature->body * 100 / psFeature->psStats->body;
			}
			else
			{
				scrFunctionResult.v.ival = 100;
			}
			break;
		case OBJ_STRUCTURE:
			psStruct = (STRUCTURE *)psObj;
			type = VAL_INT;
			//val = psStruct->body * 100 / psStruct->baseBodyPoints;
			scrFunctionResult.v.ival = psStruct->body * 100 / structureBody(psStruct);
			break;
		default:
			break;
		}
		break;
	case OBJID_BODY:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: body only valid for a droid");
			return FALSE;
		}
		type = (INTERP_TYPE)ST_BODY;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->asBits[COMP_BODY].nStat;
		break;
	case OBJID_PROPULSION:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: propulsion only valid for a droid");
			return FALSE;
		}
		type = (INTERP_TYPE)ST_PROPULSION;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->asBits[COMP_PROPULSION].nStat;
		break;
	case OBJID_WEAPON:		//TODO: only returns first weapon now
		type = (INTERP_TYPE)ST_WEAPON;
		switch (psObj->type)
		{
		case OBJ_DROID:
			if (((DROID *)psObj)->asWeaps[0].nStat == 0)
			{
				scrFunctionResult.v.ival = 0;
			}else{
				scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->asWeaps[0].nStat;
			}
			break;
		case OBJ_STRUCTURE:
			if (((STRUCTURE *)psObj)->numWeaps == 0 || ((STRUCTURE *)psObj)->asWeaps[0].nStat == 0)
			{
				scrFunctionResult.v.ival = 0;
			}else{
				scrFunctionResult.v.ival = (SDWORD)((STRUCTURE *)psObj)->asWeaps[0].nStat;
			}
			break;
		default:		//only droids and structures can have a weapon
			debug(LOG_ERROR, "scrBaseObjGet: weapon only valid for droids and structures" );
			return FALSE;
			break;
		}

		break;

	case OBJID_STRUCTSTAT:
		//droid.stat - now returns the type of structure a truck is building for droids
		if (psObj->type == OBJ_STRUCTURE)
		{
			type = (INTERP_TYPE)ST_STRUCTURESTAT;
			scrFunctionResult.v.ival = ((STRUCTURE *)psObj)->pStructureType - asStructureStats;
		}
		else if (psObj->type == OBJ_DROID)
		{
			type = (INTERP_TYPE)ST_STRUCTURESTAT;
			scrFunctionResult.v.ival = (SDWORD)((STRUCTURE_STATS *)(((DROID *)psObj)->psTarStats) - asStructureStats);
		}
		else		//Nothing else supported
		{
			debug(LOG_ERROR, "scrBaseObjGet(): .stat only valid for structures and droids");
			return FALSE;
		}

		break;

	case OBJID_TARGET:
		//added object->psTarget
		if (psObj->type == OBJ_STRUCTURE)
		{
			type = (INTERP_TYPE)ST_BASEOBJECT;
			scrFunctionResult.v.oval = ((STRUCTURE *)psObj)->psTarget[0];
		}
		else if (psObj->type == OBJ_DROID)
		{
			type = (INTERP_TYPE)ST_BASEOBJECT;
			scrFunctionResult.v.oval = ((DROID *)psObj)->psTarget;
		}
		else		//Nothing else supported
		{
			debug(LOG_ERROR, "scrBaseObjGet(): .target only valid for structures and droids");
			return FALSE;
		}

		break;
	case OBJID_GROUP:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: group only valid for a droid");
			return FALSE;
		}

		type = (INTERP_TYPE)ST_GROUP;
		scrFunctionResult.v.oval = ((DROID *)psObj)->psGroup;
		break;
	default:
		debug(LOG_ERROR, "scrBaseObjGet: unknown variable index");
		return FALSE;
		break;
	}

	// Return the value
	if (!stackPushResult(type, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrBaseObjGet: stackPushResult() failed");
		return FALSE;
	}

	return TRUE;
}


// convert a base object to a droid if it is the right type
BOOL scrObjToDroid(void)
{
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	// return NULL if not a droid
	if (psObj->type != OBJ_DROID)
	{
		psObj = NULL;
	}


	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// convert a base object to a structure if it is the right type
BOOL scrObjToStructure(void)
{
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	// return NULL if not a droid
	if (psObj->type != OBJ_STRUCTURE)
	{
		psObj = NULL;
	}

	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// convert a base object to a feature if it is the right type
BOOL scrObjToFeature(void)
{
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	// return NULL if not a droid
	if (psObj->type != OBJ_FEATURE)
	{
		psObj = NULL;
	}

	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// cache all the possible values for the last group to try
// to speed up access
static SDWORD		lgX,lgY, lgMembers, lgHealth;

// Get values from a group
BOOL scrGroupObjGet(UDWORD index)
{
	INTERP_TYPE		type;
	DROID_GROUP		*psGroup;
	DROID			*psCurr;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return FALSE;
	}

	//fix: turn off caching, since it can screw up everything if returns outdated values
	// recalculate the values if necessary
/*
	if (lgGameTime != gameTime || psScrLastGroup != psGroup)
	{
		lgGameTime = gameTime;
		psScrLastGroup = psGroup;
		lgMembers = 0;
		lgHealth = 0;
		lgX = lgY = 0;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			lgMembers += 1;
			lgX += (SDWORD)psCurr->pos.x;
			lgY += (SDWORD)psCurr->pos.y;
			lgHealth += (SDWORD)((100 * psCurr->body)/psCurr->originalBody);
		}
		if (lgMembers > 0)
		{
			lgX = lgX / lgMembers;
			lgY = lgY / lgMembers;
			lgHealth = lgHealth / lgMembers;
		}
	}
*/
	switch (index)
	{
	case GROUPID_POSX:
		lgX = 0;
		lgMembers = 0;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			lgMembers += 1;
			lgX += (SDWORD)psCurr->pos.x;
		}

		if (lgMembers > 0)
		{
			lgX = lgX / lgMembers;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = lgX;
		break;
	case GROUPID_POSY:
		lgY = 0;
		lgMembers = 0;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			lgMembers += 1;
			lgY += (SDWORD)psCurr->pos.y;
		}

		if (lgMembers > 0)
		{
			lgY = lgY / lgMembers;
		}

		type = VAL_INT;
		scrFunctionResult.v.ival = lgY;
		break;
	case GROUPID_MEMBERS:
		lgMembers = 0;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			lgMembers += 1;
		}

		type = VAL_INT;
		scrFunctionResult.v.ival = lgMembers;
		break;
	case GROUPID_HEALTH:
		lgHealth = 0;
		lgMembers = 0;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			lgMembers += 1;
			lgHealth += (SDWORD)((100 * psCurr->body)/psCurr->originalBody);
		}

		if (lgMembers > 0)
		{
			lgHealth = lgHealth / lgMembers;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = lgHealth;
		break;
	case GROUPID_TYPE:
		type = VAL_INT;
		scrFunctionResult.v.ival = psGroup->type;
		break;
	case GROUPID_CMD:
		type = ST_DROID;
		scrFunctionResult.v.oval = psGroup->psCommander;
		break;
	default:
		ASSERT( FALSE, "scrGroupObjGet: unknown variable index" );
		return FALSE;
		break;
	}

	// Return the value
	if (!stackPushResult(type, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// get the name from a stat pointer
static char *scrGetStatName(INTERP_TYPE type, UDWORD data)
{
	char	*pName = NULL;

	switch (type)
	{
	case ST_STRUCTURESTAT:
		if (data < numStructureStats)
		{
			pName = asStructureStats[data].pName;
		}
		break;
	case ST_FEATURESTAT:
		if (data < numFeatureStats)
		{
			pName = asFeatureStats[data].pName;
		}
		break;
	case ST_BODY:
		if (data < numBodyStats)
		{
			pName = asBodyStats[data].pName;
		}
		break;
	case ST_PROPULSION:
		if (data < numPropulsionStats)
		{
			pName = asPropulsionStats[data].pName;
		}
		break;
	case ST_ECM:
		if (data < numECMStats)
		{
			pName = asECMStats[data].pName;
		}
		break;
	case ST_SENSOR:
		if (data < numSensorStats)
		{
			pName = asSensorStats[data].pName;
		}
		break;
	case ST_CONSTRUCT:
		if (data < numConstructStats)
		{
			pName = asConstructStats[data].pName;
		}
		break;
	case ST_WEAPON:
		if (data < numWeaponStats)
		{
			pName = asWeaponStats[data].pName;
		}
		break;
	case ST_REPAIR:
		if (data < numRepairStats)
		{
			pName = asRepairStats[data].pName;
		}
		break;
	case ST_BRAIN:
		if (data < numBrainStats)
		{
			pName = asBrainStats[data].pName;
		}
		break;
	case ST_BASESTATS:
	case ST_COMPONENT:
		// should never have variables of this type
		break;
	default:
		break;
	}

	if (pName == NULL)
	{
		debug( LOG_ERROR, "scrGetStatName: cannot get name for a base stat" );
		abort();
	}

	return pName;
}

// default value save routine
//TODO: use union
BOOL scrValDefSave(INTERP_VAL *psVal, char *pBuffer, UDWORD *pSize)
{
	VIEWDATA	*psIntMessage;
	const char	*pName;
	RESEARCH	*psResearch;
	char		*pPos;
	DROID		*psCDroid;
	SDWORD		members;
	DROID_GROUP	*psGroup;
#ifdef _DEBUG
	BASE_OBJECT	*psObj;
#endif

	switch (psVal->type)
	{
	case ST_INTMESSAGE:
		// save the name
		psIntMessage = (VIEWDATA *)psVal->v.oval;
		if (psIntMessage != NULL)
		{
			if (pBuffer)
			{
				strcpy(pBuffer, psIntMessage->pName);
			}
			*pSize = strlen(psIntMessage->pName)+1;
		}
		else
		{
			if (pBuffer)
			{
				*pBuffer = '\0';
			}
			*pSize = 1;
		}
		break;
	case ST_BASEOBJECT:
	case ST_DROID:
	case ST_STRUCTURE:
	case ST_FEATURE:
		// just save the id
		if (pBuffer)
		{
			if (psVal->v.oval == NULL)
			{
				*((UDWORD*)pBuffer) = UDWORD_MAX;
			}
			else
			{
				*((UDWORD*)pBuffer) = ((BASE_OBJECT *)psVal->v.oval)->id;
#ifdef _DEBUG
				psObj = getBaseObjFromId(((BASE_OBJECT *)psVal->v.oval)->id);
				ASSERT( psObj == (BASE_OBJECT *)psVal->v.oval,"scrValDefSave failed to find object, continue" );
#endif
			}

			endian_udword((UDWORD*)pBuffer);
		}
		*pSize = sizeof(UDWORD);
		break;
	case ST_BASESTATS:
	case ST_COMPONENT:
	case ST_FEATURESTAT:
	case ST_STRUCTURESTAT:
	case ST_BODY:
	case ST_PROPULSION:
	case ST_ECM:
	case ST_SENSOR:
	case ST_CONSTRUCT:
	case ST_WEAPON:
	case ST_REPAIR:
	case ST_BRAIN:
		pName = scrGetStatName(psVal->type, psVal->v.ival);
		if (pName != NULL)
		{
			if (pBuffer)
			{
				strcpy(pBuffer, pName);
			}
			*pSize = strlen(pName) + 1;
		}
		else
		{
			return FALSE;
		}
		break;
	case ST_TEMPLATE:
		if (pBuffer)
		{
			if (psVal->v.oval == NULL)
			{
				*((UDWORD*)pBuffer) = UDWORD_MAX;
			}
			else
			{
				*((UDWORD*)pBuffer) = ((DROID_TEMPLATE *)psVal->v.oval)->multiPlayerID;
			}
			endian_udword((UDWORD*)pBuffer);
		}
		*pSize = sizeof(UDWORD);
		break;
	case ST_TEXTSTRING:
		if (pBuffer)
		{
			if (psVal->v.sval == NULL)
			{
				*((UDWORD*)pBuffer) = UDWORD_MAX;
			}
			else
			{
				*((UDWORD*)pBuffer) = strresGetIDfromString(psStringRes, psVal->v.sval);
			}
			endian_udword((UDWORD*)pBuffer);
		}
		*pSize = sizeof(UDWORD);
		break;
	case ST_LEVEL:
		if (psVal->v.sval != NULL)
		{
			if (pBuffer)
			{
				strcpy(pBuffer, psVal->v.sval);
			}
			*pSize = strlen(psVal->v.sval)+1;
		}
		else
		{
			if (pBuffer)
			{
				*pBuffer = '\0';
			}
			*pSize = 1;
		}
		break;
	case ST_RESEARCH:
		psResearch = (RESEARCH *)psVal->v.oval;
		if (psResearch != NULL)
		{
			if (pBuffer)
			{
				strcpy(pBuffer, psResearch->pName);
			}
			*pSize = strlen(psResearch->pName)+1;
		}
		else
		{
			if (pBuffer)
			{
				*pBuffer = '\0';
			}
			*pSize = 1;
		}
		break;
	case ST_GROUP:
		if (psVal->v.oval != NULL)
		{
			members = grpNumMembers((DROID_GROUP *)psVal->v.oval);
		}
		else
		{
			members = 0;
		}
		if (pBuffer != NULL)
		{
			pPos = pBuffer;
			psGroup = (DROID_GROUP *)psVal->v.oval;

			// store the run data
			*((SDWORD *)pPos) = psGroup->sRunData.sPos.x;
			endian_sdword((SDWORD*)pPos);
			pPos += sizeof(SDWORD);
			*((SDWORD *)pPos) = psGroup->sRunData.sPos.y;
			endian_sdword((SDWORD*)pPos);
			pPos += sizeof(SDWORD);
			*((SDWORD *)pPos) = psGroup->sRunData.forceLevel;
			endian_sdword((SDWORD*)pPos);
			pPos += sizeof(SDWORD);
			*((SDWORD *)pPos) = psGroup->sRunData.leadership;
			endian_sdword((SDWORD*)pPos);
			pPos += sizeof(SDWORD);
			*((SDWORD *)pPos) = psGroup->sRunData.healthLevel;
			endian_sdword((SDWORD*)pPos);
			pPos += sizeof(SDWORD);

			// now store the droids
			for(psCDroid=((DROID_GROUP *)psVal->v.oval)->psList; psCDroid; psCDroid=psCDroid->psGrpNext)
			{
				checkValidId(psCDroid->id);

				*((UDWORD *)pPos) = psCDroid->id;
				endian_udword((UDWORD*)pPos);

				pPos += sizeof(UDWORD);
			}
		}
		*pSize = sizeof(UDWORD) * members + sizeof(SDWORD) * 5;
		break;
	case ST_SOUND:
		if(psVal->v.ival)
		{
			// can also return NULL
			pName = sound_GetTrackName((UDWORD)psVal->v.ival);
		}
		else
		{
			pName = NULL;
		}
		if (pName == NULL)
		{
			debug(LOG_WARNING, "scrValDefSave: couldn't get sound track name");
			// just save an empty string
			pName = "";
		}
		if (pBuffer)
		{
			strcpy(pBuffer, pName);
		}
		*pSize = strlen(pName) + 1;
		break;
	case ST_STRUCTUREID:
	case ST_DROIDID:
		// just save the variable contents directly
		if (pBuffer)
		{
			*((UDWORD *)pBuffer) = psVal->v.ival;
			endian_udword((UDWORD*)pBuffer);
		}
		*pSize = sizeof(UDWORD);
		break;
  default:
    ASSERT( FALSE, "scrValDefSave: unknown script variable type for save" );
    break;
	}
	return TRUE;
}

/// default value load routine
BOOL scrValDefLoad(SDWORD version, INTERP_VAL *psVal, char *pBuffer, UDWORD size)
{
	char			*pPos;
	DROID			*psCDroid;
	SDWORD			index, members;
	UDWORD			id;
	LEVEL_DATASET	*psLevel;
	DROID_GROUP		*psGroup = NULL;
	const char              *pName;

	switch (psVal->type)
	{
	case ST_INTMESSAGE:
		if ((size == 1) &&
			(*pBuffer == 0))
		{
			psVal->v.oval = NULL;
		}
		else
		{
			psVal->v.oval = (void*)getViewData(pBuffer);
			if (psVal->v.oval == NULL)
			{
				return FALSE;
			}
		}
		break;
	case ST_BASEOBJECT:
	case ST_DROID:
	case ST_STRUCTURE:
	case ST_FEATURE:
		id = *((UDWORD *)pBuffer);
		endian_udword(&id);

		if (id == UDWORD_MAX)
		{
			psVal->v.oval = NULL;
		}
		else
		{
			psVal->v.oval = (void*)getBaseObjFromId(id);
			if (!psVal->v.oval)
			{
				debug( LOG_ERROR, "scrValDefLoad: couldn't find object id %d", id );
				abort();
			}
		}
		break;
	case ST_BASESTATS:
	case ST_COMPONENT:
		break;
	case ST_STRUCTURESTAT:
		index = getStructStatFromName(pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find structure stat %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_FEATURESTAT:
		index = getFeatureStatFromName(pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find feature stat %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_BODY:
		index = getCompFromResName(COMP_BODY, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find body component %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_PROPULSION:
		index = getCompFromResName(COMP_PROPULSION, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find propulsion component %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_ECM:
		index = getCompFromResName(COMP_ECM, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find ECM component %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_SENSOR:
		index = getCompFromResName(COMP_SENSOR, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find sensor component %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_CONSTRUCT:
		index = getCompFromResName(COMP_CONSTRUCT, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find constructor component %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_WEAPON:
		index = getCompFromResName(COMP_WEAPON, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find weapon %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_REPAIR:
		index = getCompFromResName(COMP_REPAIRUNIT, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find repair component %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_BRAIN:
		index = getCompFromResName(COMP_BRAIN, pBuffer);
		if (index == -1)
		{
			debug( LOG_ERROR, "scrValDefLoad: couldn't find repair brain %s", pBuffer );
			abort();
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_TEMPLATE:
		id = *((UDWORD *)pBuffer);
		endian_udword(&id);

		if (id == UDWORD_MAX)
		{
			psVal->v.oval = NULL;
		}
		else
		{
			psVal->v.oval = (void*)IdToTemplate(id, ANYPLAYER);
			if ((DROID_TEMPLATE*)(psVal->v.oval) == NULL)
			{
				debug( LOG_ERROR, "scrValDefLoad: couldn't find template id %d", id );
				abort();
			}
		}
		break;
	case ST_TEXTSTRING:
		id = *((UDWORD *)pBuffer);
		endian_udword(&id);
		if (id == UDWORD_MAX)
		{
			psVal->v.sval = '\0';
		}
		else
		{
			psVal->v.sval = strresGetString(psStringRes, id);
		}
		break;
	case ST_LEVEL:
		if ((size == 1) &&
			(*pBuffer == 0))
		{
			psVal->v.sval = '\0';
		}
		else
		{
			if (!levFindDataSet(pBuffer, &psLevel))
			{
				debug( LOG_ERROR, "scrValDefLoad: couldn't find level dataset %s", pBuffer );
				abort();
			}
			psVal->v.sval = psLevel->pName;
		}
		break;
	case ST_RESEARCH:
		if ((size == 1) &&
			(*pBuffer == 0))
		{
			psVal->v.oval = NULL;
		}
		else
		{
			psVal->v.oval = (void*)getResearch(pBuffer, TRUE);
			if (psVal->v.oval == NULL)
			{
				debug( LOG_ERROR, "scrValDefLoad: couldn't find research %s", pBuffer );
				abort();
			}
		}
		break;
	case ST_GROUP:
		if (psVal->v.oval == NULL)
		{
			if (!grpCreate((DROID_GROUP**)&(psVal->v.oval)))
			{
				debug( LOG_ERROR, "scrValDefLoad: out of memory" );
				abort();
				break;
			}
			grpJoin((DROID_GROUP*)(psVal->v.oval), NULL);
		}

		switch (version)
		{
			case 1:
				members = size / sizeof(UDWORD);
				break;
			case 2:
				members = (size - sizeof(SDWORD)*4) / sizeof(UDWORD);
				break;
			case 3:
				members = (size - sizeof(SDWORD)*5) / sizeof(UDWORD);
				break;
			default:
				members = 0;
				debug( LOG_ERROR, "scrValDefLoad: unsupported version %i", version);
		}
		pPos = pBuffer;
		if (version >= 2)
		{
			// load the retreat data
			psGroup = (DROID_GROUP*)(psVal->v.oval);
			endian_sdword((SDWORD*)pPos);
			psGroup->sRunData.sPos.x = *((SDWORD *)pPos);
			pPos += sizeof(SDWORD);
			endian_sdword((SDWORD*)pPos);
			psGroup->sRunData.sPos.y = *((SDWORD *)pPos);
			pPos += sizeof(SDWORD);
			endian_sdword((SDWORD*)pPos);
			psGroup->sRunData.forceLevel = (UBYTE)(*((SDWORD *)pPos));
			pPos += sizeof(SDWORD);
			endian_sdword((SDWORD*)pPos);
			psGroup->sRunData.leadership = (UBYTE)(*((SDWORD *)pPos));
			pPos += sizeof(SDWORD);
		}
		if (version >= 3)
		{
			endian_sdword((SDWORD*)pPos);
			psGroup->sRunData.healthLevel = (UBYTE)(*((SDWORD *)pPos));
			pPos += sizeof(SDWORD);
		}

		// load the droids
		while (members > 0)
		{
			endian_udword((UDWORD*)pPos);
			id = *((UDWORD *) pPos);
			psCDroid = (DROID *)getBaseObjFromId(id);
			if (!psCDroid)
			{
				debug( LOG_ERROR, "scrValDefLoad: couldn't find object id %d", id );
				abort();
			}
			else
			{
				grpJoin((DROID_GROUP*)(psVal->v.oval), psCDroid);
			}

			pPos += sizeof(UDWORD);
			members -= 1;
		}
		break;
	case ST_SOUND:
		// find audio id

		// don't use sound if it's disabled
		if (audio_Disabled())
		{
			psVal->v.ival = NO_SOUND;
			break;
		}

		pName = pBuffer;
		index = audio_GetTrackID( pName );
		if (index == SAMPLE_NOT_FOUND)
		{
			// find empty id and set track vals
			index = audio_SetTrackVals(pName, FALSE, 100, 1800);
			if (!index)			//this is a NON fatal error. 
			{
				// We can't find filename of the sound for some reason.
				debug(LOG_ERROR, "Sound ID not available %s not found", pName);
				break;
			}
		}
		psVal->v.ival = index;
		break;
	case ST_STRUCTUREID:
	case ST_DROIDID:
	default:
		// just set the contents directly
		psVal->v.ival = *((SDWORD *)pBuffer);
		endian_sdword(&psVal->v.ival);
		break;
	}

	return TRUE;
}
