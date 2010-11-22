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

// ---------------------------------------------------------------------
// STATIC SUPPORT FUNCTIONS
UDWORD	selSelectAllUnits		( UDWORD player, BOOL bOnScreen );
UDWORD	selSelectAllSameProp	( UDWORD player, PROPULSION_TYPE propType,
									BOOL bOnScreen );
UDWORD	selSelectAllCombat		( UDWORD player, BOOL bOnScreen);
UDWORD	selSelectAllDamaged		( UDWORD player, BOOL bOnScreen);
UDWORD	selSelectAllSame		( UDWORD player, BOOL bOnScreen);
UDWORD	selNameSelect			( char *droidName, UDWORD player, BOOL bOnScreen );
// ---------------------------------------------------------------------
/*
	Selects the units of a given player according to given criteria.
	It is also possible to request whether the units be onscreen or not.
*/

static DROID	*psOldRD = NULL;	// pointer to last selected repair unit
static DROID	*psOldNS = NULL;
static STRUCTURE *psOldStruct = NULL;

UDWORD	selDroidSelection( UDWORD	player, SELECTION_CLASS droidClass,
						  SELECTIONTYPE droidType, BOOL bOnScreen )
{
UDWORD	retVal;
char	selInfo[255];

	/* So far, we haven't selected any */
	retVal = 0;

	/* Establish the class of selection */
	switch(droidClass)
	{
	case	DS_ALL_UNITS:
		retVal = selSelectAllUnits(player,bOnScreen);
		break;
	case	DS_BY_TYPE:
		switch(droidType)
		{
		case	DST_VTOL:
			retVal = selSelectAllSameProp(player,PROPULSION_TYPE_LIFT,bOnScreen);
			break;
		case	DST_HOVER:
			retVal = selSelectAllSameProp(player,PROPULSION_TYPE_HOVER,bOnScreen);
			break;
		case	DST_WHEELED:
			retVal = selSelectAllSameProp(player,PROPULSION_TYPE_WHEELED,bOnScreen);
			break;
		case	DST_TRACKED:
			retVal = selSelectAllSameProp(player,PROPULSION_TYPE_TRACKED,bOnScreen);
			break;
		case	DST_HALF_TRACKED:
			retVal = selSelectAllSameProp(player,PROPULSION_TYPE_HALF_TRACKED,bOnScreen);
			break;
		case	DST_ALL_COMBAT:
			retVal = selSelectAllCombat(player,bOnScreen);
			break;
		case	DST_ALL_DAMAGED:
			retVal = selSelectAllDamaged(player,bOnScreen);
			break;
		case DST_ALL_SAME:
			retVal = selSelectAllSame(player,bOnScreen);
			break;
		default:
			ASSERT( false,"Invalid selection type in uniDroidSelection" );
		}
		break;
	default:
		ASSERT( false,"Invalid selection attempt in uniDroidSelection" );
		break;
	}

	/* Send back the return value */
	snprintf(selInfo, sizeof(selInfo), ngettext("%u unit selected", "%u units selected", retVal), retVal);
	addConsoleMessage(selInfo, RIGHT_JUSTIFY,SYSTEM_MESSAGE);
	return retVal;
}

// ---------------------------------------------------------------------
// Selects all units owned by the player - onscreen toggle.
UDWORD	selSelectAllUnits( UDWORD player, BOOL bOnScreen )
{
DROID	*psDroid;
UDWORD	count;

	selDroidDeselect(player);
	/* Go thru' all */
	for(psDroid = apsDroidLists[player],count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			/* Do we care about them being on screen? */
			if(bOnScreen ? droidOnScreen(psDroid,0) : true)
			{
				/* can select everything except transporters */
				if(psDroid->droidType != DROID_TRANSPORTER)
				{
//					psDroid->selected = true;
					SelectDroid(psDroid);
					count++;
				}
			}
		}

	return(count);
}
// ---------------------------------------------------------------------
// Selects all units owned by the player of a certain propulsion type.
// On Screen toggle.
UDWORD	selSelectAllSameProp( UDWORD player, PROPULSION_TYPE propType,
						   BOOL bOnScreen )
{
PROPULSION_STATS	*psPropStats;
DROID	*psDroid;
UDWORD	count;

	selDroidDeselect(player);
	/* Go thru' them all */
	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
	{
		/* Is on screen important */
		if(bOnScreen ? droidOnScreen(psDroid,0) : true)
		{
			/* Get the propulsion type */
			psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
			ASSERT( psPropStats != NULL,
				"moveUpdateUnit: invalid propulsion stats pointer" );
			/* Same as that asked for - don't want Transporters*/
			if ( psPropStats->propulsionType == propType && psDroid->droidType != DROID_TRANSPORTER)
			{
				psDroid->selected = true;
				count++;
			}
		}
	}

	return(selNumSelected(player));
}
// ---------------------------------------------------------------------
// Selects all units owned by the player that have a weapon. On screen
// toggle.
UDWORD	selSelectAllCombat( UDWORD player, BOOL bOnScreen)
{
DROID	*psDroid;
UDWORD	count;

	selDroidDeselect(player);
	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			/* Does it have a weapon? */
			//if(psDroid->numWeaps)
            if(psDroid->asWeaps[0].nStat > 0)
			{
		  		/* Is on screen relevant? */
				if(bOnScreen ? droidOnScreen(psDroid,0) : true)
				{
                    //we don't want to get the Transporter
	 				if(psDroid->droidType != DROID_TRANSPORTER)
					{
						psDroid->selected = true;
						count ++;
					}
				}
			}
		}
	return(count);
}
// ---------------------------------------------------------------------
// Selects all damaged units - on screen toggle.
UDWORD	selSelectAllDamaged( UDWORD player, BOOL bOnScreen)
{
DROID	*psDroid;
UDWORD	damage;
UDWORD	count;

	selDroidDeselect(player);
	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			/* Get present percent of damage level */
			damage = PERCENT(psDroid->body,psDroid->originalBody);

			/* Less than threshold? */
			if(damage<REPAIRLEV_LOW)
			{
				/* Is on screen relevant? */
				if(bOnScreen ? droidOnScreen(psDroid,0) : true)
				{
                    //we don't want to get the Transporter
	 				if(psDroid->droidType != DROID_TRANSPORTER)
                    {
//    					psDroid->selected = true;
						SelectDroid(psDroid);
	    				count ++;
                    }
				}
			}
		}
	return(count);
}
// ---------------------------------------------------------------------
// Deselects all units for the player
UDWORD	selDroidDeselect( UDWORD player )
{
UDWORD	count;
DROID	*psDroid;

	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			if(psDroid->selected)
			{
				count++;
			}
//			psDroid->selected = false;
			DeSelectDroid(psDroid);
		}
	return(count);
}
// ---------------------------------------------------------------------
// Lets you know how many are selected for a given player
UDWORD	selNumSelected( UDWORD player )
{
UDWORD	count;
DROID	*psDroid;

	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			if(psDroid->selected)
			{
				count++;
			}
		}
	return(count);
}

// ---------------------------------------------------------------------
// Selects all units the same as the one(s) selected
UDWORD	selSelectAllSame( UDWORD player, BOOL bOnScreen)
{


DROID	*psDroid;
UDWORD	count;

	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			if(psDroid->selected)
			{
				count+=selNameSelect(psDroid->aName, player, bOnScreen);
			}
		}
	return(selNumSelected(player));



}
// ---------------------------------------------------------------------
// sub-function - selects all units with same name as one passed in
UDWORD	selNameSelect( char *droidName, UDWORD player, BOOL bOnScreen )
{

DROID	*psDroid;
UDWORD	count;

 	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			if(!psDroid->selected)
			{
				if(bOnScreen ? droidOnScreen(psDroid,0) : true)
				{
					if(!strcmp(droidName,psDroid->aName))
					{
//						psDroid->selected = true;
						SelectDroid(psDroid);
						count++;
					}
				}
			}
		}
	if(count)
		return(count+1);
	else
		return(count);

}

// ffs am
// ---------------------------------------------------------------------
void	selNextSpecifiedUnit(UDWORD unitType)
{
DROID	*psCurr;
DROID	*psResult;
DROID	*psFirst;
BOOL	bLaterInList, bMatch;

	for(psCurr = apsDroidLists[selectedPlayer],psFirst = NULL,psResult = NULL,bLaterInList = false;
		psCurr && !psResult; psCurr = psCurr->psNext)
	{
		//if( psCurr->droidType == (SDWORD)unitType )
        //exceptions - as always...
        bMatch = false;
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
        else if(psCurr->droidType == (SDWORD)unitType)
        {
            bMatch = true;
        }
        if (bMatch)
		{
		 	/* Always store away the first one we find */
			if(!psFirst)
			{
				psFirst = psCurr;
			}

			if(psCurr == psOldRD)
			{
				bLaterInList = true;
			}

			/* Nothing previously found... */
			if(!psOldRD)
			{
				psResult = psCurr;
			}

			/* Only select is this isn't the old one and it's further on in list */
			else if(psCurr!=psOldRD && bLaterInList)
			{
				psResult = psCurr;
			}

		 }
	}

	/* Did we get one? */
	if(!psResult)
	{
		/* was there at least one - the first one? Resetting */
		if(psFirst)
		{
			psResult = psFirst;
		}
	}

	if(psResult && !psResult->died)
	{
	 	selDroidDeselect(selectedPlayer);
//		psResult->selected = true;
		SelectDroid(psResult);
		if(getWarCamStatus())
		{
			camToggleStatus();			 // messy - fix this
	//		setViewPos(map_coord(psCentreDroid->pos.x), map_coord(psCentreDroid->pos.y));
			processWarCam(); //odd, but necessary
			camToggleStatus();				// messy - FIXME
		}
		else
			if(!getWarCamStatus())
			{
//				camToggleStatus();
				/* Centre display on him if warcam isn't active */
				setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), true);
			}
		psOldRD = psResult;
	}
	else
	{
		switch(unitType)
		{
		case	DROID_REPAIR:
			addConsoleMessage(_("Unable to locate any repair units!"),LEFT_JUSTIFY,SYSTEM_MESSAGE);
			break;
		case	DROID_CONSTRUCT:
			addConsoleMessage(_("Unable to locate any Trucks!"),LEFT_JUSTIFY,SYSTEM_MESSAGE);
			break;
		case	DROID_SENSOR:
			addConsoleMessage(_("Unable to locate any Sensor Units!"),LEFT_JUSTIFY,SYSTEM_MESSAGE);
			break;
		case	DROID_COMMAND:
			addConsoleMessage(_("Unable to locate any Commanders!"),LEFT_JUSTIFY,SYSTEM_MESSAGE);
			break;
		}
	}
}
// ---------------------------------------------------------------------
void	selNextUnassignedUnit( void )
{
DROID	*psCurr;
DROID	*psResult;
DROID	*psFirst;
BOOL	bLaterInList;

	for(psCurr = apsDroidLists[selectedPlayer],psFirst = NULL,psResult = NULL,bLaterInList = false;
		psCurr && !psResult; psCurr = psCurr->psNext)
	{
		/* Only look at unselected ones */
		if(psCurr->group==UBYTE_MAX)
		{

		  	/* Keep a record of first one */
			if(!psFirst)
			{
				psFirst = psCurr;
			}

			if(psCurr == psOldNS)
			{
				bLaterInList = true;
			}

			/* First one...? */
			if(!psOldNS)
			{
				psResult = psCurr;
			}

			/* Dont choose same one again */
			else if(psCurr!=psOldNS && bLaterInList)
			{
				psResult = psCurr;
			}

		}
	}

	/* If we didn't get one - then select first one */
	if(!psResult)
	{
		if(psFirst)
		{
			psResult = psFirst;
		}
	}

	if(psResult && !psResult->died)
	{
	 	selDroidDeselect(selectedPlayer);
//		psResult->selected = true;
		SelectDroid(psResult);
		if(getWarCamStatus())
		{
			camToggleStatus();			 // messy - fix this
	//		setViewPos(map_coord(psCentreDroid->pos.x), map_coord(psCentreDroid->pos.y));
			processWarCam(); //odd, but necessary
			camToggleStatus();				// messy - FIXME
		}
		else
			if(!getWarCamStatus())
			{
//				camToggleStatus();
				/* Centre display on him if warcam isn't active */
				setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), true);
			}
		psOldNS = psResult;
	}
	else
	{
		addConsoleMessage(_("Unable to locate any repair units!"),LEFT_JUSTIFY,SYSTEM_MESSAGE);
	}
}
// ---------------------------------------------------------------------
void	selNextSpecifiedBuilding(UDWORD	structType)
{
STRUCTURE	*psCurr;
STRUCTURE	*psResult;
STRUCTURE	*psFirst;
BOOL		bLaterInList;

	/* Firstly, start coughing if the type is invalid */
	ASSERT(structType <= NUM_DIFF_BUILDINGS, "Invalid structure type %u in selNextSpecifiedBuilding", structType);

	for(psCurr = apsStructLists[selectedPlayer], psFirst = NULL,psResult = NULL,bLaterInList = false;
		psCurr && !psResult; psCurr = psCurr->psNext)
		{
			if(psCurr->pStructureType->type == structType)
			{
				if(!psFirst)
				{
					psFirst = psCurr;
				}
				if(psCurr == psOldStruct)
				{
					bLaterInList = true;
				}
				if(!psOldStruct)
				{
					psResult = psCurr;
				}
				else if(psCurr!=psOldStruct && bLaterInList)
				{
					psResult = psCurr;
				}
			}
		}

		if(!psResult)
		{
			if(psFirst)
			{
				psResult = psFirst;
			}
		}

		if(psResult && !psResult->died)
		{
			if(getWarCamStatus())
			{
				camToggleStatus();
			}
			setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), false);
			psOldStruct = psResult;
		}
		else
		{
			// Can't find required building
			addConsoleMessage("Cannot find required building!",LEFT_JUSTIFY,SYSTEM_MESSAGE);
		}
}



// ---------------------------------------------------------------------

// see if a commander is the n'th command droid
static BOOL droidIsCommanderNum(DROID *psDroid, SDWORD n)
{
	DROID	*psCurr;
	SDWORD	numLess;

	if (psDroid->droidType != DROID_COMMAND)
	{
		return false;
	}

	numLess = 0;
	for(psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if ((psCurr->droidType == DROID_COMMAND) &&
			(psCurr->id < psDroid->id))
		{
			numLess += 1;
		}
	}

	if (numLess == (n - 1))
	{
		return true;
	}

	return false;
}

// select the n'th command droid
void selCommander(SDWORD n)
{
	DROID	*psCurr;

	for(psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr=psCurr->psNext)
	{
		if (droidIsCommanderNum(psCurr, n))
		{
			if (!psCurr->selected)
			{
				clearSelection();
				psCurr->selected = true;
			}
			else
			{
				clearSelection();
				psCurr->selected = true;

				// this horrible bit of code is taken from activateGroupAndMove
				// and sets the camera position to that of the commander

				if(getWarCamStatus())
				{
					camToggleStatus();			 // messy - fix this
					processWarCam(); //odd, but necessary
					camToggleStatus();				// messy - FIXME
				}
				else
				if(!getWarCamStatus())
				{
					/* Centre display on him if warcam isn't active */
					setViewPos(map_coord(psCurr->pos.x), map_coord(psCurr->pos.y), true);
				}

			}
			setSelectedCommander((UDWORD)n);
			return;
		}
	}
}
