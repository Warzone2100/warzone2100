/*
	Selection.c
	Alex McLean, Pumpkin studios, EIDOS.
	Attempt to rationalise the unit selection procedure and
	also necessary as we now need to return the number of
	units selected according to specified criteria.
*/

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"

#include "objects.h"
#include "base.h"
#include "droiddef.h"
#include "statsdef.h"
#include "text.h"
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
// EXTERNALLY REFERENCED FUNCTIONS
UDWORD	selDroidSelection		( UDWORD	player, SELECTION_CLASS droidClass,
									SELECTIONTYPE droidType, BOOL bOnScreen );
UDWORD	selDroidDeselect		( UDWORD player );
UDWORD	selNumSelected			( UDWORD player );

// ---------------------------------------------------------------------
// STATIC SUPPORT FUNCTIONS
UDWORD	selSelectAllUnits		( UDWORD player, BOOL bOnScreen );
UDWORD	selSelectAllSameProp	( UDWORD player, PROPULSION_TYPE propType,
									BOOL bOnScreen );
UDWORD	selSelectAllCombat		( UDWORD player, BOOL bOnScreen);
UDWORD	selSelectAllDamaged		( UDWORD player, BOOL bOnScreen);
UDWORD	selSelectAllSame		( UDWORD player, BOOL bOnScreen);
UDWORD	selNameSelect			( STRING *droidName, UDWORD player, BOOL bOnScreen );
// ---------------------------------------------------------------------
/*
	Selects the units of a given player according to given criteria.
	It is also possible to request whether the units be onscreen or not.
*/

DROID	*psOldRD = NULL;	// pointer to last selected repair unit
DROID	*psOldNS = NULL;
STRUCTURE	*psOldStruct = NULL;

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
			retVal = selSelectAllSameProp(player,LIFT,bOnScreen);
			break;
		case	DST_HOVER:
			retVal = selSelectAllSameProp(player,HOVER,bOnScreen);
			break;
		case	DST_WHEELED:
			retVal = selSelectAllSameProp(player,WHEELED,bOnScreen);
			break;
		case	DST_TRACKED:
			retVal = selSelectAllSameProp(player,TRACKED,bOnScreen);
			break;
		case	DST_HALF_TRACKED:
			retVal = selSelectAllSameProp(player,HALF_TRACKED,bOnScreen);
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
			ASSERT( FALSE,"Invalid selection type in uniDroidSelection" );
		}
		break;
	default:
		ASSERT( FALSE,"Invalid selection attempt in uniDroidSelection" );
		break;
	}

	/* Send back the return value */
	sprintf(selInfo,strresGetString(psStringRes,STR_GAM_UNITSEL),retVal);
	addConsoleMessage(selInfo,RIGHT_JUSTIFY);
	return(retVal);
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
			if(bOnScreen ? droidOnScreen(psDroid,0) : TRUE)
			{
				/* can select everything except transporters */
				if(psDroid->droidType != DROID_TRANSPORTER)
				{
//					psDroid->selected = TRUE;
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
		if(bOnScreen ? droidOnScreen(psDroid,0) : TRUE)
		{
			/* Get the propulsion type */
			psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
			ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
				"moveUpdateUnit: invalid propulsion stats pointer" );
			/* Same as that asked for - don't want Transporters*/
			if ( psPropStats->propulsionType == propType AND psDroid->droidType != DROID_TRANSPORTER)
			{
				psDroid->selected = TRUE;
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
				if(bOnScreen ? droidOnScreen(psDroid,0) : TRUE)
				{
                    //we don't want to get the Transporter
	 				if(psDroid->droidType != DROID_TRANSPORTER)
					{
						psDroid->selected = TRUE;
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
				if(bOnScreen ? droidOnScreen(psDroid,0) : TRUE)
				{
                    //we don't want to get the Transporter
	 				if(psDroid->droidType != DROID_TRANSPORTER)
                    {
//    					psDroid->selected = TRUE;
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
//			psDroid->selected = FALSE;
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
UDWORD	selNameSelect( STRING *droidName, UDWORD player, BOOL bOnScreen )
{

DROID	*psDroid;
UDWORD	count;

 	for(psDroid = apsDroidLists[player], count = 0;
		psDroid; psDroid = psDroid->psNext)
		{
			if(!psDroid->selected)
			{
				if(bOnScreen ? droidOnScreen(psDroid,0) : TRUE)
				{
					if(!strcmp(droidName,psDroid->aName))
					{
//						psDroid->selected = TRUE;
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

	for(psCurr = apsDroidLists[selectedPlayer],psFirst = NULL,psResult = NULL,bLaterInList = FALSE;
		psCurr AND !psResult; psCurr = psCurr->psNext)
	{
		//if( psCurr->droidType == (SDWORD)unitType )
        //exceptions - as always...
        bMatch = FALSE;
        if (unitType == DROID_CONSTRUCT)
        {
            if (psCurr->droidType == DROID_CONSTRUCT OR
                psCurr->droidType == DROID_CYBORG_CONSTRUCT)
            {
                bMatch = TRUE;
            }
        }
        else if (unitType == DROID_REPAIR)
        {
            if (psCurr->droidType == DROID_REPAIR OR
                psCurr->droidType == DROID_CYBORG_REPAIR)
            {
                bMatch = TRUE;
            }
        }
        else if(psCurr->droidType == (SDWORD)unitType)
        {
            bMatch = TRUE;
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
				bLaterInList = TRUE;
			}

			/* Nothing previously found... */
			if(!psOldRD)
			{
				psResult = psCurr;
			}

			/* Only select is this isn't the old one and it's further on in list */
			else if(psCurr!=psOldRD AND bLaterInList)
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

	if(psResult AND !psResult->died)
	{
	 	selDroidDeselect(selectedPlayer);
//		psResult->selected = TRUE;
		SelectDroid(psResult);
		if(getWarCamStatus())
		{
			camToggleStatus();			 // messy - fix this
	//		setViewPos(psCentreDroid->x>>TILE_SHIFT,psCentreDroid->y>>TILE_SHIFT);
			processWarCam(); //odd, but necessary
			camToggleStatus();				// messy - FIXME
		}
		else
			if(!getWarCamStatus())
			{
//				camToggleStatus();
				/* Centre display on him if warcam isn't active */
				setViewPos(psResult->x>>TILE_SHIFT,psResult->y>>TILE_SHIFT,TRUE);
			}
		psOldRD = psResult;
	}
	else
	{
		switch(unitType)
		{
		case	DROID_REPAIR:
			addConsoleMessage(strresGetString(psStringRes,STR_GAM_REPNOTFOUND),LEFT_JUSTIFY);
			break;
		case	DROID_CONSTRUCT:
			addConsoleMessage(strresGetString(psStringRes,STR_GAM_CONNOTFOUND),LEFT_JUSTIFY);
			break;
		case	DROID_SENSOR:
			addConsoleMessage(strresGetString(psStringRes,STR_GAM_SENNOTFOUND),LEFT_JUSTIFY);
			break;
		case	DROID_COMMAND:
			addConsoleMessage(strresGetString(psStringRes,STR_GAM_COMNOTFOUND),LEFT_JUSTIFY);
			break;
		}
	}
}
#if 0
// ---------------------------------------------------------------------
void	selNextRepairUnit( void )
{
DROID	*psCurr;
DROID	*psResult;
DROID	*psFirst;
BOOL	bLaterInList;

	for(psCurr = apsDroidLists[selectedPlayer],psFirst = NULL,psResult = NULL,bLaterInList = FALSE;
		psCurr AND !psResult; psCurr = psCurr->psNext)
	{
		if( psCurr->droidType == DROID_REPAIR OR
            psCurr->droidType == DROID_CYBORG_REPAIR )
		{

		 	/* Always store away the first one we find */
			if(!psFirst)
			{
				psFirst = psCurr;
			}

			if(psCurr == psOldRD)
			{
				bLaterInList = TRUE;
			}

			/* Nothing previously found... */
			if(!psOldRD)
			{
				psResult = psCurr;
			}

			/* Only select is this isn't the old one and it's further on in list */
			else if(psCurr!=psOldRD AND bLaterInList)
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

	if(psResult AND !psResult->died)
	{
	 	selDroidDeselect(selectedPlayer);
//		psResult->selected = TRUE;
		SelectDroid(psResult);
		if(getWarCamStatus())
		{
			camToggleStatus();			 // messy - fix this
	//		setViewPos(psCentreDroid->x>>TILE_SHIFT,psCentreDroid->y>>TILE_SHIFT);
			processWarCam(); //odd, but necessary
			camToggleStatus();				// messy - FIXME
		}
		else
			if(!getWarCamStatus())
			{
//				camToggleStatus();
				/* Centre display on him if warcam isn't active */
				setViewPos(psResult->x>>TILE_SHIFT,psResult->y>>TILE_SHIFT,TRUE);
			}
		psOldRD = psResult;
	}
	else
	{
		addConsoleMessage(strresGetString(psStringRes,STR_GAM_REPNOTFOUND),LEFT_JUSTIFY);
	}
}
// ---------------------------------------------------------------------
#endif
// ---------------------------------------------------------------------
void	selNextUnassignedUnit( void )
{
DROID	*psCurr;
DROID	*psResult;
DROID	*psFirst;
BOOL	bLaterInList;

	for(psCurr = apsDroidLists[selectedPlayer],psFirst = NULL,psResult = NULL,bLaterInList = FALSE;
		psCurr AND !psResult; psCurr = psCurr->psNext)
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
				bLaterInList = TRUE;
			}

			/* First one...? */
			if(!psOldNS)
			{
				psResult = psCurr;
			}

			/* Dont choose same one again */
			else if(psCurr!=psOldNS AND bLaterInList)
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

	if(psResult AND !psResult->died)
	{
	 	selDroidDeselect(selectedPlayer);
//		psResult->selected = TRUE;
		SelectDroid(psResult);
		if(getWarCamStatus())
		{
			camToggleStatus();			 // messy - fix this
	//		setViewPos(psCentreDroid->x>>TILE_SHIFT,psCentreDroid->y>>TILE_SHIFT);
			processWarCam(); //odd, but necessary
			camToggleStatus();				// messy - FIXME
		}
		else
			if(!getWarCamStatus())
			{
//				camToggleStatus();
				/* Centre display on him if warcam isn't active */
				setViewPos(psResult->x>>TILE_SHIFT,psResult->y>>TILE_SHIFT,TRUE);
			}
		psOldNS = psResult;
	}
	else
	{
		addConsoleMessage(strresGetString(psStringRes,STR_GAM_REPNOTFOUND),LEFT_JUSTIFY);
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
	ASSERT( structType>=REF_HQ AND structType<=NUM_DIFF_BUILDINGS,
		"Invalid structure type in selNextSpecifiedBuilding" );

	for(psCurr = apsStructLists[selectedPlayer], psFirst = NULL,psResult = NULL,bLaterInList = FALSE;
		psCurr AND !psResult; psCurr = psCurr->psNext)
		{
			if(psCurr->pStructureType->type == structType)
			{
				if(!psFirst)
				{
					psFirst = psCurr;
				}
				if(psCurr == psOldStruct)
				{
					bLaterInList = TRUE;
				}
				if(!psOldStruct)
				{
					psResult = psCurr;
				}
				else if(psCurr!=psOldStruct AND bLaterInList)
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

		if(psResult AND !psResult->died)
		{
			if(getWarCamStatus())
			{
				camToggleStatus();
			}
			setViewPos(psResult->x>>TILE_SHIFT,psResult->y>>TILE_SHIFT,FALSE);
			psOldStruct = psResult;
		}
		else
		{
			// Can't find required building
			addConsoleMessage("Cannot find required building!",LEFT_JUSTIFY);
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
		return FALSE;
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
		return TRUE;
	}

	return FALSE;
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
				psCurr->selected = TRUE;
			}
			else
			{
				clearSelection();
				psCurr->selected = TRUE;

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
					setViewPos(psCurr->x>>TILE_SHIFT,psCurr->y>>TILE_SHIFT,TRUE);
				}

			}
			setSelectedCommander((UDWORD)n);
			return;
		}
	}
}

// ---------------------------------------------------------------------




