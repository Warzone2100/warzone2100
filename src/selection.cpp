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
/**
 * @file selection.c
 * Attempt to rationalise the unit selection procedure and
 * also necessary as we now need to return the number of
 * units selected according to specified criteria.
 * Alex McLean, Pumpkin studios, EIDOS.
*/

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/strres.h"

#include "objects.h"
#include "basedef.h"
#include "droiddef.h"
#include "statsdef.h"
#include "geometry.h"
#include "console.h"
#include "selection.h"
#include "hci.h"
#include "map.h"
#include "selection.h"
#include "display3d.h"
#include "warcam.h"
#include "display.h"
#include "qtscript.h"
#include <functional>

template <typename T>
static unsigned selSelectUnitsIf(unsigned player, T condition, bool onlyOnScreen)
{
	unsigned count = 0;

	selDroidDeselect(player);

	// Go through all.
	for (DROID *psDroid = apsDroidLists[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		bool shouldSelect = (!onlyOnScreen || droidOnScreen(psDroid, 0)) &&
		                    condition(psDroid);
		count += shouldSelect;
		if (shouldSelect && !psDroid->selected && !psDroid->flags.test(OBJECT_FLAG_UNSELECTABLE))
		{
			SelectDroid(psDroid);
		}
		else if (!shouldSelect && psDroid->selected)
		{
			DeSelectDroid(psDroid);
		}
	}

	return count;
}

template <typename T, typename U>
static unsigned selSelectUnitsIf(unsigned player, T condition, U value, bool onlyOnScreen)
{
	return selSelectUnitsIf(player, std::bind2nd(std::ptr_fun(condition), value), onlyOnScreen);
}

static bool selTransporter(DROID *droid)
{
	return isTransporter(droid);
}
static bool selTrue(DROID *droid)
{
	return !selTransporter(droid);
}
static bool selProp(DROID *droid, PROPULSION_TYPE prop)
{
	return asPropulsionStats[droid->asBits[COMP_PROPULSION]].propulsionType == prop && !selTransporter(droid);
}
static bool selPropArmed(DROID *droid, PROPULSION_TYPE prop)
{
	return asPropulsionStats[droid->asBits[COMP_PROPULSION]].propulsionType == prop && vtolFull(droid) && !selTransporter(droid);
}
static bool selType(DROID *droid, DROID_TYPE type)
{
	return droid->droidType == type;
}
static bool selCombat(DROID *droid)
{
	return droid->asWeaps[0].nStat > 0 && !selTransporter(droid);
}
static bool selCombatLand(DROID *droid)
{
	PROPULSION_TYPE type = asPropulsionStats[droid->asBits[COMP_PROPULSION]].propulsionType;
	return droid->asWeaps[0].nStat > 0 && (type == PROPULSION_TYPE_WHEELED ||
	                                       type == PROPULSION_TYPE_HALF_TRACKED ||
	                                       type == PROPULSION_TYPE_TRACKED ||
	                                       type == PROPULSION_TYPE_HOVER ||
	                                       type == PROPULSION_TYPE_LEGGED);
}
static bool selCombatCyborg(DROID *droid)
{
	PROPULSION_TYPE type = asPropulsionStats[droid->asBits[COMP_PROPULSION]].propulsionType;
	return droid->asWeaps[0].nStat > 0 && type == PROPULSION_TYPE_LEGGED;
}
static bool selDamaged(DROID *droid)
{
	return PERCENT(droid->body, droid->originalBody) < REPAIRLEV_LOW && !selTransporter(droid);
}

// ---------------------------------------------------------------------
// Deselects all units for the player
unsigned int selDroidDeselect(unsigned int player)
{
	unsigned int count = 0;

	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			count++;
			DeSelectDroid(psDroid);
		}
	}

	return count;
}

// ---------------------------------------------------------------------
// Lets you know how many are selected for a given player
unsigned int selNumSelected(unsigned int player)
{
	unsigned int count = 0;

	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			count++;
		}
	}

	return count;
}

// ---------------------------------------------------------------------
// sub-function - selects all units with same name as one passed in
static void selNameSelect(char *droidName, unsigned int player, bool bOnScreen)
{
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (!psDroid->selected)
		{
			if (!bOnScreen || droidOnScreen(psDroid, 0))
			{
				if (!strcmp(droidName, psDroid->aName))
				{
					SelectDroid(psDroid);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------
// Selects all units the same as the one(s) selected
static unsigned int selSelectAllSame(unsigned int player, bool bOnScreen)
{
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			selNameSelect(psDroid->aName, player, bOnScreen);
		}
	}

	return selNumSelected(player);
}

// ffs am
// ---------------------------------------------------------------------
void selNextSpecifiedUnit(DROID_TYPE unitType)
{
	static DROID *psOldRD = nullptr; // pointer to last selected repair unit
	DROID *psResult = nullptr, *psFirst = nullptr;
	bool bLaterInList = false;

	for (DROID *psCurr = apsDroidLists[selectedPlayer]; psCurr && !psResult; psCurr = psCurr->psNext)
	{
		//exceptions - as always...
		bool bMatch = false;
		if (unitType == DROID_CONSTRUCT)
		{
			if (psCurr->droidType == DROID_CONSTRUCT ||
			    psCurr->droidType == DROID_CYBORG_CONSTRUCT)
			{
				bMatch = true;
			}
		}
		else if (unitType == DROID_REPAIR)
		{
			if (psCurr->droidType == DROID_REPAIR ||
			    psCurr->droidType == DROID_CYBORG_REPAIR)
			{
				bMatch = true;
			}
		}
		else if (psCurr->droidType == unitType)
		{
			bMatch = true;
		}
		if (bMatch)
		{
			/* Always store away the first one we find */
			if (!psFirst)
			{
				psFirst = psCurr;
			}

			if (psCurr == psOldRD)
			{
				bLaterInList = true;
			}

			/* Nothing previously found... */
			if (!psOldRD)
			{
				psResult = psCurr;
			}

			/* Only select is this isn't the old one and it's further on in list */
			else if (psCurr != psOldRD && bLaterInList)
			{
				psResult = psCurr;
			}
		}
	}

	/* Did we get one? */
	if (!psResult && psFirst)
	{
		psResult = psFirst;
	}

	if (psResult && !psResult->died)
	{
		selDroidDeselect(selectedPlayer);
		SelectDroid(psResult);
		if (getWarCamStatus())
		{
			camToggleStatus(); // messy - fix this
			// setViewPos(map_coord(psCentreDroid->pos.x), map_coord(psCentreDroid->pos.y));
			processWarCam(); //odd, but necessary
			camToggleStatus(); // messy - FIXME
		}
		else
		{
			// camToggleStatus();
			/* Centre display on him if warcam isn't active */
			setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), true);
		}
		psOldRD = psResult;
	}
	else
	{
		switch (unitType)
		{
		case	DROID_REPAIR:
			addConsoleMessage(_("Unable to locate any repair units!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
			break;
		case	DROID_CONSTRUCT:
			addConsoleMessage(_("Unable to locate any Trucks!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
			break;
		case	DROID_SENSOR:
			addConsoleMessage(_("Unable to locate any Sensor Units!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
			break;
		case	DROID_COMMAND:
			addConsoleMessage(_("Unable to locate any Commanders!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		default:
			break;
		}
	}
}

// ---------------------------------------------------------------------
void selNextUnassignedUnit()
{
	static DROID *psOldNS = nullptr;
	DROID *psResult = nullptr, *psFirst = nullptr;
	bool bLaterInList = false;

	for (DROID *psCurr = apsDroidLists[selectedPlayer]; psCurr && !psResult; psCurr = psCurr->psNext)
	{
		/* Only look at unselected ones */
		if (psCurr->group == UBYTE_MAX)
		{
			/* Keep a record of first one */
			if (!psFirst)
			{
				psFirst = psCurr;
			}

			if (psCurr == psOldNS)
			{
				bLaterInList = true;
			}

			/* First one...? */
			if (!psOldNS)
			{
				psResult = psCurr;
			}

			/* Dont choose same one again */
			else if (psCurr != psOldNS && bLaterInList)
			{
				psResult = psCurr;
			}
		}
	}

	/* If we didn't get one - then select first one */
	if (!psResult && psFirst)
	{
		psResult = psFirst;
	}

	if (psResult && !psResult->died)
	{
		selDroidDeselect(selectedPlayer);
		SelectDroid(psResult);
		if (getWarCamStatus())
		{
			camToggleStatus(); // messy - fix this
			// setViewPos(map_coord(psCentreDroid->pos.x), map_coord(psCentreDroid->pos.y));
			processWarCam(); //odd, but necessary
			camToggleStatus(); // messy - FIXME
		}
		else
		{
			// camToggleStatus();
			/* Centre display on him if warcam isn't active */
			setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), true);
		}
		psOldNS = psResult;
	}
	else
	{
		addConsoleMessage(_("Unable to locate any repair units!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

// ---------------------------------------------------------------------
void selNextSpecifiedBuilding(STRUCTURE_TYPE structType)
{
	STRUCTURE *psResult = nullptr, *psOldStruct = nullptr, *psFirst = nullptr;
	bool bLaterInList = false;

	/* Firstly, start coughing if the type is invalid */
	ASSERT(structType <= NUM_DIFF_BUILDINGS, "Invalid structure type %u", structType);

	for (STRUCTURE *psCurr = apsStructLists[selectedPlayer]; psCurr && !psResult; psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == structType && psCurr->status == SS_BUILT)
		{
			if (!psFirst)
			{
				psFirst = psCurr;
			}
			if (psCurr->selected)
			{
				bLaterInList = true;
				psOldStruct = psCurr;
			}
			else if (bLaterInList)
			{
				psResult = psCurr;
			}
		}
	}

	if (!psResult && psFirst)
	{
		psResult = psFirst;
	}

	if (psResult && !psResult->died)
	{
		if (getWarCamStatus())
		{
			camToggleStatus();
		}
		setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), false);
		if (psOldStruct)
		{
			psOldStruct->selected = false;
		}
		psResult->selected = true;
		triggerEventSelected();
		jsDebugSelected(psResult);
	}
	else
	{
		// Can't find required building
		addConsoleMessage("Cannot find required building!", LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

// ---------------------------------------------------------------------
// see if a commander is the n'th command droid
static bool droidIsCommanderNum(DROID *psDroid, SDWORD n)
{
	if (psDroid->droidType != DROID_COMMAND)
	{
		return false;
	}

	int numLess = 0;
	for (DROID *psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
	{
		if ((psCurr->droidType == DROID_COMMAND) && (psCurr->id < psDroid->id))
		{
			numLess++;
		}
	}

	return (numLess == (n - 1));
}

// select the n'th command droid
void selCommander(int n)
{
	for (DROID *psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if (droidIsCommanderNum(psCurr, n))
		{
			if (!psCurr->selected && !psCurr->flags.test(OBJECT_FLAG_UNSELECTABLE))
			{
				clearSelection();
				psCurr->selected = true;
			}
			else if (!psCurr->flags.test(OBJECT_FLAG_UNSELECTABLE))
			{
				clearSelection();
				psCurr->selected = true;

				// this horrible bit of code is taken from activateGroupAndMove
				// and sets the camera position to that of the commander

				if (getWarCamStatus())
				{
					camToggleStatus(); // messy - fix this
					processWarCam(); // odd, but necessary
					camToggleStatus(); // messy - FIXME
				}
				else
				{
					/* Centre display on him if warcam isn't active */
					setViewPos(map_coord(psCurr->pos.x), map_coord(psCurr->pos.y), true);
				}
			}
			return;
		}
	}
}

// ---------------------------------------------------------------------
/*
   Selects the units of a given player according to given criteria.
   It is also possible to request whether the units be onscreen or not.
   */
unsigned int selDroidSelection(unsigned int player, SELECTION_CLASS droidClass, SELECTIONTYPE droidType, bool bOnScreen)
{
	/* So far, we haven't selected any */
	unsigned int retVal = 0;

	/* Establish the class of selection */
	switch (droidClass)
	{
	case DS_ALL_UNITS:
		retVal = selSelectUnitsIf(player, selTrue, bOnScreen);
		break;
	case DS_BY_TYPE:
		switch (droidType)
		{
		case DST_VTOL:
			retVal = selSelectUnitsIf(player, selProp, PROPULSION_TYPE_LIFT, bOnScreen);
			break;
		case DST_VTOL_ARMED:
			retVal = selSelectUnitsIf(player, selPropArmed, PROPULSION_TYPE_LIFT, bOnScreen);
			break;
		case DST_HOVER:
			retVal = selSelectUnitsIf(player, selProp, PROPULSION_TYPE_HOVER, bOnScreen);
			break;
		case DST_WHEELED:
			retVal = selSelectUnitsIf(player, selProp, PROPULSION_TYPE_WHEELED, bOnScreen);
			break;
		case DST_TRACKED:
			retVal = selSelectUnitsIf(player, selProp, PROPULSION_TYPE_TRACKED, bOnScreen);
			break;
		case DST_HALF_TRACKED:
			retVal = selSelectUnitsIf(player, selProp, PROPULSION_TYPE_HALF_TRACKED, bOnScreen);
			break;
		case DST_CYBORG:
			retVal = selSelectUnitsIf(player, selProp, PROPULSION_TYPE_LEGGED, bOnScreen);
			break;
		case DST_ENGINEER:
			retVal = selSelectUnitsIf(player, selType, DROID_CYBORG_CONSTRUCT, bOnScreen);
			break;
		case DST_MECHANIC:
			retVal = selSelectUnitsIf(player, selType, DROID_CYBORG_REPAIR, bOnScreen);
			break;
		case DST_TRANSPORTER:
			retVal = selSelectUnitsIf(player, selTransporter, bOnScreen);
			break;
		case DST_REPAIR_TANK:
			retVal = selSelectUnitsIf(player, selType, DROID_REPAIR, bOnScreen);
			break;
		case DST_SENSOR:
			retVal = selSelectUnitsIf(player, selType, DROID_SENSOR, bOnScreen);
			break;
		case DST_TRUCK:
			retVal = selSelectUnitsIf(player, selType, DROID_CONSTRUCT, bOnScreen);
			break;
		case DST_ALL_COMBAT:
			retVal = selSelectUnitsIf(player, selCombat, bOnScreen);
			break;
		case DST_ALL_COMBAT_LAND:
			retVal = selSelectUnitsIf(player, selCombatLand, bOnScreen);
			break;
		case DST_ALL_COMBAT_CYBORG:
			retVal = selSelectUnitsIf(player, selCombatCyborg, bOnScreen);
			break;
		case DST_ALL_DAMAGED:
			retVal = selSelectUnitsIf(player, selDamaged, bOnScreen);
			break;
		case DST_ALL_SAME:
			retVal = selSelectAllSame(player, bOnScreen);
			break;
		default:
			ASSERT(false, "Invalid selection type");
		}
		break;
	default:
		ASSERT(false, "Invalid selection attempt");
		break;
	}

	CONPRINTF(ngettext("%u unit selected", "%u units selected", retVal), retVal);

	return retVal;
}
