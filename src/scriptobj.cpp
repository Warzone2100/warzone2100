/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/endian_hack.h"
#include "lib/framework/strres.h"
#include "objects.h"

#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptobj.h"
#include "group.h"
#include "lib/gamelib/gtime.h"
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
bool scrBaseObjGet(UDWORD index)
{
	INTERP_TYPE		type = VAL_VOID;
	BASE_OBJECT		*psObj;
	DROID			*psDroid;
	STRUCTURE		*psStruct;
	FEATURE			*psFeature;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrBaseObjGet: stackPopParams failed");
		return false;
	}

	// Check this is a valid pointer
	ASSERT_OR_RETURN(false, psObj, "Passed a NULL pointer to a base object");
	ASSERT_OR_RETURN(false, psObj->type == OBJ_DROID || psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_FEATURE,
	                 "Invalid object %p of type %d", psObj, psObj->type);

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
			return false;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = ((DROID *)psObj)->order.type;
		if (scrFunctionResult.v.ival == DORDER_GUARD && ((DROID *)psObj)->order.psObj == nullptr)
		{
			scrFunctionResult.v.ival = DORDER_NONE;
		}
		break;
	//new member variable
	case OBJID_ACTION:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: action only valid for a droid");
			return false;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->action;
		break;
	//new member variable - if droid is selected (humans only)
	case OBJID_SELECTED:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: selected only valid for a droid");
			return false;
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
			return false;
		}
		break;
	case OBJID_ORDERX:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: order only valid for a droid");
			return false;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = ((DROID *)psObj)->order.pos.x;
		break;
	case OBJID_ORDERY:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: order only valid for a droid");
			return false;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = ((DROID *)psObj)->order.pos.y;
		break;
	case OBJID_DROIDTYPE:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: droidType only valid for a droid");
			return false;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->droidType;
		break;
	case OBJID_CLUSTERID:
		if (psObj->type == OBJ_FEATURE)
		{
			debug(LOG_ERROR, "scrBaseObjGet: clusterID not valid for features");
			return false;
		}
		type = VAL_INT;
		scrFunctionResult.v.ival = 0;
		ASSERT(false, "unsupported wzscript feature");
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
			return false;
		}
		type = (INTERP_TYPE)ST_BODY;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->asBits[COMP_BODY];
		break;
	case OBJID_PROPULSION:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: propulsion only valid for a droid");
			return false;
		}
		type = (INTERP_TYPE)ST_PROPULSION;
		scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->asBits[COMP_PROPULSION];
		break;
	case OBJID_WEAPON:		//TODO: only returns first weapon now
		type = (INTERP_TYPE)ST_WEAPON;
		switch (psObj->type)
		{
		case OBJ_DROID:
			if (((DROID *)psObj)->asWeaps[0].nStat == 0)
			{
				scrFunctionResult.v.ival = 0;
			}
			else
			{
				scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->asWeaps[0].nStat;
			}
			break;
		case OBJ_STRUCTURE:
			if (((STRUCTURE *)psObj)->numWeaps == 0 || ((STRUCTURE *)psObj)->asWeaps[0].nStat == 0)
			{
				scrFunctionResult.v.ival = 0;
			}
			else
			{
				scrFunctionResult.v.ival = (SDWORD)((STRUCTURE *)psObj)->asWeaps[0].nStat;
			}
			break;
		default:		//only droids and structures can have a weapon
			debug(LOG_ERROR, "scrBaseObjGet: weapon only valid for droids and structures");
			return false;
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
			scrFunctionResult.v.ival = ((DROID *)psObj)->order.psStats - asStructureStats;
		}
		else		//Nothing else supported
		{
			debug(LOG_ERROR, "scrBaseObjGet(): .stat only valid for structures and droids");
			return false;
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
			scrFunctionResult.v.oval = ((DROID *)psObj)->order.psObj;
		}
		else		//Nothing else supported
		{
			debug(LOG_ERROR, "scrBaseObjGet(): .target only valid for structures and droids");
			return false;
		}
		break;
	case OBJID_GROUP:
		if (psObj->type != OBJ_DROID)
		{
			debug(LOG_ERROR, "scrBaseObjGet: group only valid for a droid");
			return false;
		}
		type = (INTERP_TYPE)ST_GROUP;
		scrFunctionResult.v.oval = ((DROID *)psObj)->psGroup;
		break;
	case OBJID_HITPOINTS:
		type = VAL_INT;
		switch (psObj->type)
		{
		case OBJ_DROID:
			scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->body;
			break;

		case OBJ_STRUCTURE:
			scrFunctionResult.v.ival = (SDWORD)((STRUCTURE *)psObj)->body;
			break;

		case OBJ_FEATURE:
			scrFunctionResult.v.ival = (SDWORD)((FEATURE *)psObj)->body;
			break;

		default:
			debug(LOG_ERROR, "scrBaseObjGet: unknown object type");
			return false;
			break;
		}
		break;
	case OBJID_ORIG_HITPOINTS:
		type = VAL_INT;
		switch (psObj->type)
		{
		case OBJ_DROID:
			scrFunctionResult.v.ival = (SDWORD)((DROID *)psObj)->originalBody;
			break;
		case OBJ_STRUCTURE:
			scrFunctionResult.v.ival = (SDWORD)structureBody((STRUCTURE *)psObj);
			break;
		case OBJ_FEATURE:
			scrFunctionResult.v.ival = ((FEATURE *)psObj)->psStats->body;
			break;
		default:
			debug(LOG_ERROR, "scrBaseObjGet: unknown object type");
			return false;
			break;
		}
		break;
	default:
		debug(LOG_ERROR, "scrBaseObjGet: unknown variable index");
		return false;
		break;
	}

	// Return the value
	if (!stackPushResult(type, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrBaseObjGet: stackPushResult() failed");
		return false;
	}

	return true;
}


// convert a base object to a droid if it is the right type
bool scrObjToDroid()
{
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		return false;
	}

	// return NULL if not a droid
	if (psObj->type != OBJ_DROID)
	{
		psObj = nullptr;
	}

	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// convert a base object to a structure if it is the right type
bool scrObjToStructure()
{
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		return false;
	}

	// return NULL if not a droid
	if (psObj->type != OBJ_STRUCTURE)
	{
		psObj = nullptr;
	}

	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// convert a base object to a feature if it is the right type
bool scrObjToFeature()
{
	BASE_OBJECT		*psObj;

	if (!stackPopParams(1, ST_BASEOBJECT, &psObj))
	{
		return false;
	}

	// return NULL if not a droid
	if (psObj->type != OBJ_FEATURE)
	{
		psObj = nullptr;
	}

	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_FEATURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// cache all the possible values for the last group to try
// to speed up access
static SDWORD		lgX, lgY, lgMembers, lgHealth;

// Get values from a weapon
bool scrWeaponObjGet(UDWORD index)
{
	INTERP_TYPE		type;
	SDWORD			weapIndex;

	if (!stackPopParams(1, ST_WEAPON, &weapIndex))
	{
		return false;
	}

	switch (index)
	{
	case WEAPID_LONG_RANGE:
		type = VAL_INT;
		scrFunctionResult.v.ival = asWeaponStats[weapIndex].base.maxRange;
		break;
	case WEAPID_LONG_HIT:
		type = VAL_INT;
		scrFunctionResult.v.ival = asWeaponStats[weapIndex].base.hitChance;
		break;
	case WEAPID_DAMAGE:
		type = VAL_INT;
		scrFunctionResult.v.ival = asWeaponStats[weapIndex].base.damage;
		break;
	case WEAPID_FIRE_PAUSE:
		type = VAL_INT;
		scrFunctionResult.v.ival = asWeaponStats[weapIndex].base.firePause;
		break;
	case WEAPID_RELOAD_TIME:
		type = VAL_INT;
		scrFunctionResult.v.ival = asWeaponStats[weapIndex].base.reloadTime;
		break;
	case WEAPID_NUM_ROUNDS:
		type = VAL_INT;
		scrFunctionResult.v.ival = asWeaponStats[weapIndex].base.numRounds;
		break;
	default:
		ASSERT(false, "unknown variable index");
		return false;
		break;
	}

	// Return the value
	if (!stackPushResult(type, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Get values from a group
bool scrGroupObjGet(UDWORD index)
{
	INTERP_TYPE		type;
	DROID_GROUP		*psGroup;
	DROID			*psCurr;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return false;
	}

	switch (index)
	{
	case GROUPID_POSX:
		lgX = 0;
		lgMembers = 0;
		for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
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
		for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
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
		for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			lgMembers += 1;
		}

		type = VAL_INT;
		scrFunctionResult.v.ival = lgMembers;
		break;
	case GROUPID_HEALTH:
		lgHealth = 0;
		lgMembers = 0;
		for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			lgMembers += 1;
			lgHealth += (SDWORD)((100 * psCurr->body) / psCurr->originalBody);
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
		type = (INTERP_TYPE)ST_DROID;
		scrFunctionResult.v.oval = psGroup->psCommander;
		break;
	default:
		ASSERT(false, "scrGroupObjGet: unknown variable index");
		return false;
		break;
	}

	// Return the value
	if (!stackPushResult(type, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// get the name from a stat pointer
static const QString scrGetStatName(INTERP_TYPE type, UDWORD data)
{
	QString pName;

	switch ((unsigned)type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
	{
	case ST_STRUCTURESTAT:
		if (data < numStructureStats)
		{
			pName = asStructureStats[data].id;
		}
		break;
	case ST_FEATURESTAT:
		if (data < numFeatureStats)
		{
			pName = asFeatureStats[data].id;
		}
		break;
	case ST_BODY:
		if (data < numBodyStats)
		{
			pName = asBodyStats[data].id;
		}
		break;
	case ST_PROPULSION:
		if (data < numPropulsionStats)
		{
			pName = asPropulsionStats[data].id;
		}
		break;
	case ST_ECM:
		if (data < numECMStats)
		{
			pName = asECMStats[data].id;
		}
		break;
	case ST_SENSOR:
		if (data < numSensorStats)
		{
			pName = asSensorStats[data].id;
		}
		break;
	case ST_CONSTRUCT:
		if (data < numConstructStats)
		{
			pName = asConstructStats[data].id;
		}
		break;
	case ST_WEAPON:
		if (data < numWeaponStats)
		{
			pName = asWeaponStats[data].id;
		}
		break;
	case ST_REPAIR:
		if (data < numRepairStats)
		{
			pName = asRepairStats[data].id;
		}
		break;
	case ST_BRAIN:
		if (data < numBrainStats)
		{
			pName = asBrainStats[data].id;
		}
		break;
	case ST_BASESTATS:
	case ST_COMPONENT:
		// should never have variables of this type
		break;
	default:
		break;
	}

	ASSERT(!pName.isEmpty(), "cannot get name for a base stat");

	return pName;
}

// default value save routine
//TODO: use union
bool scrValDefSave(INTERP_VAL *psVal, WzConfig &ini)
{
	VIEWDATA	*psIntMessage;
	QString		pName;
	RESEARCH	*psResearch;
	DROID		*psCDroid;

	switch ((unsigned)psVal->type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
	{
	case ST_INTMESSAGE:
		// save the name
		psIntMessage = (VIEWDATA *)psVal->v.oval;
		if (psIntMessage != nullptr)
		{
			ini.setValue("data", psIntMessage->name);
		}
		break;
	case ST_BASEOBJECT:
	case ST_DROID:
	case ST_STRUCTURE:
	case ST_FEATURE:
		// just save the id
		if (psVal->v.oval && ((BASE_OBJECT *)psVal->v.oval)->died <= NOT_CURRENT_LIST)
		{
			ini.setValue("data", QVariant(((BASE_OBJECT *)psVal->v.oval)->id));
		}
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
		if (!pName.isEmpty())
		{
			ini.setValue("data", QString(pName));
		}
		break;
	case ST_TEMPLATE:
		if (psVal->v.oval)
		{
			ini.setValue("data", ((DROID_TEMPLATE *)psVal->v.oval)->multiPlayerID);
		}
		break;
	case ST_TEXTSTRING:
		{
			const char *const idStr = psVal->v.sval ? strresGetIDfromString(psStringRes, psVal->v.sval) : nullptr;
			if (idStr)
			{
				ini.setValue("data", QString(idStr));
			}
			break;
		}
	case ST_LEVEL:
		if (psVal->v.sval)
		{
			ini.setValue("data", QString(psVal->v.sval));
		}
		break;
	case ST_RESEARCH:
		psResearch = (RESEARCH *)psVal->v.oval;
		if (psResearch && !psResearch->id.isEmpty())
		{
			ini.setValue("data", psResearch->id);
			ASSERT(psResearch == getResearch(getID(psResearch)), "Research %s not found!", getID(psResearch));
		}
		break;
	case ST_GROUP:
		{
			DROID_GROUP *const psGroup = (DROID_GROUP *)psVal->v.oval;
			if (psGroup)
			{
				const int members = psGroup->getNumMembers();
				QStringList droids;
				for (psCDroid = psGroup->psList; psCDroid; psCDroid = psCDroid->psGrpNext)
				{
					checkValidId(psCDroid->id);
					droids.push_back(QString::number(psCDroid->id));
				}
				ini.setValue("members", QVariant(members));
				if (!droids.empty())
				{
					ini.setValue("data", droids);
				}
				ini.setVector2i("runpos", psGroup->sRunData.sPos);
				ini.setValue("forceLevel", QVariant(psGroup->sRunData.forceLevel));
				ini.setValue("leadership", QVariant(psGroup->sRunData.leadership));
				ini.setValue("healthLevel", QVariant(psGroup->sRunData.healthLevel));
			}
			break;
		}
	case ST_SOUND:
		if (psVal->v.ival)
		{
			// can also return NULL
			pName = sound_GetTrackName((UDWORD)psVal->v.ival);
		}
		if (pName.isEmpty())
		{
			debug(LOG_WARNING, "Could not get sound track name");
		}
		else
		{
			ini.setValue("data", pName);
		}
		break;
	case ST_STRUCTUREID:
	case ST_DROIDID:
		ini.setValue("data", QVariant(psVal->v.ival));
		break;
	default:
		ASSERT(false, "Unknown script variable type for save");
		break;
	}
	return true;
}

/// default value load routine
bool scrValDefLoad(INTERP_VAL *psVal, WzConfig &ini)
{
	DROID			*psCDroid;
	SDWORD			index, members;
	UDWORD			id;
	LEVEL_DATASET	*psLevel;
	DROID_GROUP		*psGroup = nullptr;

	switch ((unsigned)psVal->type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
	{
	case ST_INTMESSAGE:
		if (ini.contains("data"))
		{
			psVal->v.oval = (void *)getViewData(ini.value("data").toString().toUtf8().constData());
		}
		else
		{
			psVal->v.oval = nullptr;
		}
		break;
	case ST_BASEOBJECT:
	case ST_DROID:
	case ST_STRUCTURE:
	case ST_FEATURE:
		if (ini.contains("data"))
		{
			psVal->v.oval = (void *)getBaseObjFromId(ini.value("data").toInt());
		}
		else
		{
			psVal->v.oval = nullptr;
		}
		break;
	case ST_BASESTATS:
	case ST_COMPONENT:
		break;
	case ST_STRUCTURESTAT:
		index = 0;
		if (ini.contains("data"))
		{
			index = getStructStatFromName(ini.value("data").toString().toUtf8().constData());
			if (index == -1)
			{
				debug(LOG_FATAL, "Could not find stat");
				index = 0;
			}
		}
		psVal->v.ival = index;
		break;
	case ST_FEATURESTAT:
		index = 0;
		if (ini.contains("data"))
		{
			index = getFeatureStatFromName(ini.value("data").toString().toUtf8().constData());
			if (index == -1)
			{
				debug(LOG_FATAL, "Could not find stat");
				index = 0;
			}
		}
		psVal->v.ival = index;
		break;
	case ST_BODY:
		index = getCompFromName(COMP_BODY, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find body component");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_PROPULSION:
		index = getCompFromName(COMP_PROPULSION, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find propulsion component");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_ECM:
		index = getCompFromName(COMP_ECM, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find ECM component");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_SENSOR:
		index = getCompFromName(COMP_SENSOR, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find sensor component");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_CONSTRUCT:
		index = getCompFromName(COMP_CONSTRUCT, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find constructor component");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_WEAPON:
		index = getCompFromName(COMP_WEAPON, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find weapon");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_REPAIR:
		index = getCompFromName(COMP_REPAIRUNIT, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find repair component");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_BRAIN:
		index = getCompFromName(COMP_BRAIN, ini.value("data").toString());
		if (index == -1)
		{
			debug(LOG_FATAL, "Could not find repair brain");
			index = 0;
		}
		psVal->v.ival = index;
		break;
	case ST_TEMPLATE:
		psVal->v.oval = nullptr;
		if (ini.contains("data"))
		{
			// FIXME: Ugh. Find a better way to show full template info
			psVal->v.oval = (void *)IdToTemplate(ini.value("data").toInt(), ANYPLAYER);
			if ((DROID_TEMPLATE *)(psVal->v.oval) == nullptr)
			{
				debug(LOG_FATAL, "Could not find template %d", ini.value("data").toInt());
			}
		}
		break;
	case ST_TEXTSTRING:
		psVal->v.sval = nullptr;
		if (ini.contains("data"))
		{
			psVal->v.sval = strdup(ini.value("data").toString().toUtf8().constData());
		}
		break;
	case ST_LEVEL:
		psVal->v.sval = nullptr;
		if (ini.contains("data"))
		{
			psLevel = levFindDataSet(ini.value("data").toString().toUtf8().constData());
			if (psLevel == nullptr)
			{
				debug(LOG_FATAL, "Could not find level dataset");
				return false;	// FIXME: Why are we saying fatal, if this isn't?
			}
			psVal->v.sval = psLevel->pName;
		}
		break;
	case ST_RESEARCH:
		psVal->v.oval = nullptr;
		if (ini.contains("data"))
		{
			QString research = ini.value("data").toString();
			if (!research.isEmpty())
			{
				psVal->v.oval = (void *)getResearch(research.toUtf8().constData());
				ASSERT_OR_RETURN(false, psVal->v.oval, "Could not find research %s", research.toUtf8().constData());
			}
		}
		break;
	case ST_GROUP:
		if (psVal->v.oval == nullptr)
		{
			DROID_GROUP *tmp = grpCreate();
			tmp->add(nullptr);
			psVal->v.oval = tmp;
		}
		psGroup = (DROID_GROUP *)(psVal->v.oval);
		members = ini.value("members", 0).toInt();
		if (psGroup && members > 0)
		{
			QStringList droids = ini.value("data").toStringList();

			// load the retreat data
			psGroup->sRunData.sPos = ini.vector2i("runpos");
			psGroup->sRunData.forceLevel = ini.value("forceLevel").toInt();
			psGroup->sRunData.leadership = ini.value("leadership").toInt();
			psGroup->sRunData.healthLevel = ini.value("healthLevel").toInt();

			// load the droids
			while (members > 0)
			{
				id = droids.takeLast().toInt();
				psCDroid = (DROID *)getBaseObjFromId(id);
				if (!psCDroid)
				{
					debug(LOG_ERROR, "Could not find object id %d", id);
				}
				else
				{
					((DROID_GROUP *)(psVal->v.oval))->add(psCDroid);
				}
				members--;
			}
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

		index = audio_GetTrackID(ini.value("data").toString().toUtf8().constData());
		if (index == SAMPLE_NOT_FOUND)
		{
			// find empty id and set track vals
			QString soundname = ini.value("data").toString();
			index = audio_SetTrackVals(soundname.toUtf8().constData(), false, 100, 1800);
			if (!index)			// this is a NON fatal error.
			{
				// We can't find filename of the sound for some reason.
				debug(LOG_ERROR, "Sound ID not available %s not found", soundname.toUtf8().constData());
				break;
			}
		}
		psVal->v.ival = index;
		break;
	case ST_STRUCTUREID:
	case ST_DROIDID:
	default:
		// just set the contents directly
		psVal->v.ival = ini.value("data").toInt();
		break;
	}

	return true;
}
