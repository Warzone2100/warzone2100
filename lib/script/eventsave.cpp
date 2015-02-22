/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
 * EvntSave.c
 *
 * Save the state of the event system.
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/wzconfig.h"
#include <string.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/endian_hack.h"
#include "lib/framework/string_ext.h"
#include "script.h"
#include "eventsave.h"

// save the context information for the script system
static bool eventSaveContext(WzConfig &ini)
{
	int numVars, numContext = 0;
	UDWORD hashedName;

	// go through the context list
	for (SCRIPT_CONTEXT *psCCont = psContList; psCCont != NULL; psCCont = psCCont->psNext)
	{
		// save the context info
		if (!resGetHashfromData("SCRIPT", psCCont->psCode, &hashedName))
		{
			debug(LOG_FATAL, "Could not find script resource id");
			return false;
		}
		numVars = psCCont->psCode->numGlobals + psCCont->psCode->arraySize;

		ini.beginGroup("context_" + QString::number(numContext));
		ini.setValue("context", hashedName);
		ini.setValue("numVars", numVars);
		ini.setValue("release", psCCont->release);
		ini.beginGroup("var");

		// save the context variables
		int countVar = 0;
		for (VAL_CHUNK *psCVals = psCCont->psGlobals; psCVals != NULL; psCVals = psCVals->psNext)
		{
			for (int i = 0; i < CONTEXT_VALS; i++)
			{
				INTERP_VAL *psVal = psCVals->asVals + i;

				ASSERT(psVal->type < SWORD_MAX, "Variable type number %d too big", (int)psVal->type);

				ini.beginGroup(QString::number(countVar));
				ini.setValue("type", QVariant(psVal->type));
				ini.setValue("typename", QString(scriptTypeToString(psVal->type))); // for debugging

				// store the variable value
				if (psVal->type == VAL_STRING)
				{
					ini.setValue("data", QString(psVal->v.sval));
				}
				else if (psVal->type == VAL_BOOL)
				{
					ini.setValue("data", QVariant((bool)psVal->v.bval));
				}
				else if (psVal->type == VAL_FLOAT)
				{
					ini.setValue("data", QVariant((float)psVal->v.fval));
				}
				else if (psVal->type == VAL_OBJ_GETSET || psVal->type == VAL_FUNC_EXTERN)
				{
					ini.setValue("data", QString("n/a"));
				}
				else if (psVal->type < VAL_USERTYPESTART)
				{
					ini.setValue("data", QVariant(psVal->v.ival));
				}
				else
				{
					// user defined type
					SCR_VAL_SAVE saveFunc = asScrTypeTab[psVal->type - VAL_USERTYPESTART].saveFunc;

					ASSERT(saveFunc != NULL, "No save function for type %d", psVal->type);

					if (!saveFunc(psVal, ini))
					{
						debug(LOG_FATAL, "Could not get user defined variable value");
						return false;
					}
				}

				numVars -=1;
				countVar++;
				ini.endGroup();
				if (numVars <= 0)
				{
					// done all the variables
					ASSERT(psCVals->psNext == NULL, "Number of context variables does not match the script code");
					break;
				}
			}
		}
		ASSERT(numVars == 0, "Number of context variables does not match the script code (%d)", numVars);
		ini.endGroup();
		ini.endGroup();
		numContext++;
	}

	// actually store how many contexts have been saved
	ini.setValue("general/contexts", QVariant(numContext));

	return true;
}

// load the context information for the script system
static bool eventLoadContext(WzConfig &ini)
{
	SDWORD				numVars, numContext;
	SCRIPT_CONTEXT		*psCCont;
	SCR_VAL_LOAD		loadFunc;
	UDWORD				hashedName;
	SCRIPT_CODE			*psCode;
	CONTEXT_RELEASE			release;
	INTERP_VAL			*psVal, data;

	// get the number of contexts in the save file
	numContext = ini.value("general/contexts", 0).toInt();

	if (numContext == 0)
	{
		debug(LOG_FATAL, "No script contexts found -- failed to load script data");
		return false;
	}

	// go through the contexts
	for (int context = 0; context < numContext; context++)
	{
		ini.beginGroup("context_" + QString::number(context));
		hashedName = ini.value("context").toUInt();
		numVars = ini.value("numVars").toInt();
		release = (CONTEXT_RELEASE)ini.value("release").toInt();

		psCode = (SCRIPT_CODE*)resGetDataFromHash("SCRIPT", hashedName);

		// create the context
		if (!eventNewContext(psCode, release, &psCCont))
		{
			debug(LOG_FATAL, "Failed to create new context");
			return false;
		}
		if (numVars != psCode->numGlobals + psCode->arraySize)
		{
			ASSERT(false, "Context %d of %d: Number of context variables (%d) does not match the script code (%d)",
			       context, numContext, numVars, psCode->numGlobals + psCode->arraySize);
			return false;
		}

		// bit of a hack this - note the id of the context to link it to the triggers
		psContList->id = context;
		ini.beginGroup("var");

		// set the context variables
		for (int i = 0; i < numVars; i++)
		{
			ini.beginGroup(QString::number(i));
			// get the variable type
			INTERP_TYPE type = (INTERP_TYPE)ini.value("type").toInt();

			// get the variable value
			if (type < VAL_USERTYPESTART)
			{
				data.type = type;

				switch (type)
				{
				case VAL_BOOL:
					data.v.bval = ini.value("data").toBool();
					break;
				case VAL_FLOAT:
					data.v.fval = ini.value("data").toFloat();
					break;
				case VAL_INT:
				case VAL_TRIGGER:
				case VAL_EVENT:
				case VAL_VOID:
				case VAL_OPCODE:
				case VAL_PKOPCODE:
					data.v.ival = ini.value("data").toInt();
					break;
				case VAL_STRING:
					data.v.sval = (char*)malloc(MAXSTRLEN);
					strcpy(data.v.sval, ini.value("var/" + QString::number(i) + "/data").toString().toUtf8().constData());
					break;
				case VAL_OBJ_GETSET:
				case VAL_FUNC_EXTERN:
					// do nothing
					break;
				default:
					ASSERT(false, "Invalid internal type");
				}

				// set the value in the context
				if (!eventSetContextVar(psCCont, i, &data))
				{
					debug(LOG_FATAL, "Could not set variable value");
					return false;
				}
			}
			else
			{
				// user defined type
				loadFunc = asScrTypeTab[type - VAL_USERTYPESTART].loadFunc;

				ASSERT(loadFunc, "No load function for type %d", type);

				// get the value pointer so that the loadFunc can write directly
				// into the variables data space.
				if (!eventGetContextVal(psCCont, i, &psVal))
				{
					debug(LOG_FATAL, "Could not find variable %d in context %d", i, context);
					return false;
				}
				if (!loadFunc(psVal, ini))
				{
					debug(LOG_FATAL, "Could not get variable value context %d, variable %d", context, i);
					return false;
				}
			}
			ini.endGroup();
		}
		ini.endGroup();
		ini.endGroup();
	}
	return true;
}

// return the index of a context
static bool eventGetContextIndex(SCRIPT_CONTEXT *psContext, SDWORD *pIndex)
{
	int index = 0;

	for (SCRIPT_CONTEXT *psCurr = psContList; psCurr!= NULL; psCurr = psCurr->psNext)
	{
		if (psCurr == psContext)
		{
			*pIndex = index;
			return true;
		}
		index += 1;
	}
	return false;
}

// find a context from it's id number
static bool eventFindContext(SDWORD id, SCRIPT_CONTEXT **ppsContext)
{
	for (SCRIPT_CONTEXT *psCurr = psContList; psCurr!= NULL; psCurr=psCurr->psNext)
	{
		if (psCurr->id == id)
		{
			*ppsContext = psCurr;
			return true;
		}
	}
	return false;
}

// save a list of triggers
static bool eventSaveTriggerList(ACTIVE_TRIGGER *psList, QString tname, WzConfig &ini)
{
	int numTriggers = 0, context = 0;

	for (ACTIVE_TRIGGER *psCurr = psList; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (!eventGetContextIndex(psCurr->psContext, &context))
		{
			debug(LOG_FATAL, "Could not find context");
			return false;
		}
		ini.beginGroup(tname + "_" + QString::number(numTriggers));
		ini.setValue("time", QVariant(psCurr->testTime));
		ini.setValue("context", QVariant(context));
		ini.setValue("type", QVariant(psCurr->type));
		ini.setValue("trigger", QVariant(psCurr->trigger));
		ini.setValue("event", QVariant(psCurr->event));
		ini.setValue("offset", QVariant(psCurr->offset));
		ini.endGroup();
		numTriggers++;
	}
	ini.setValue("general/num" + tname, QVariant(numTriggers));
	return true;
}

// load a list of triggers
static bool eventLoadTriggerList(WzConfig &ini, QString tname)
{
	UDWORD			event, offset, time;
	int			numTriggers, context, type, trigger;
	SCRIPT_CONTEXT		*psContext;

	numTriggers = ini.value("general/num" + tname).toInt();

	for (int i = 0; i < numTriggers; i++)
	{
		ini.beginGroup(tname + "_" + QString::number(i));
		time = ini.value("time").toInt();
		context = ini.value("context").toInt();
		if (!eventFindContext(context, &psContext))
		{
			debug(LOG_FATAL, "could not find context");
			return false;
		}
		type = ini.value("type").toInt();
		trigger = ini.value("trigger").toInt();
		event = ini.value("event").toInt();
		offset = ini.value("offset").toInt();
		if (!eventLoadTrigger(time, psContext, type, trigger, event, offset))
		{
			debug(LOG_FATAL, "Failed to create trigger");
			return false;
		}
		ini.endGroup();
	}
	return true;
}

// Save the state of the event system
bool eventSaveState(const char *pFilename)
{
	WzConfig ini(pFilename);
	if (!eventSaveContext(ini) || !eventSaveTriggerList(psTrigList, "trig", ini) || !eventSaveTriggerList(psCallbackList, "callback", ini))
	{
		return false;
	}
	return true;
}

// Load the state of the event system
bool eventLoadState(const char *pFilename)
{
	WzConfig ini(pFilename, WzConfig::ReadOnly);
	// load the event contexts
	if (!eventLoadContext(ini) || !eventLoadTriggerList(ini, "trig") || !eventLoadTriggerList(ini, "callback"))
	{
		return false;
	}
	return true;
}
