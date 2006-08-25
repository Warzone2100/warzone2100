/*
 * Power.c
 *
 * Store PlayerPower and other power related stuff!
 *
 */
#include "objectdef.h"
#include "power.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "objmem.h"
#include "frontend.h"

#include "multiplay.h"
#include "multiint.h"

#include "feature.h"
#include "structure.h"
#include "mission.h"
#include "research.h"
#include "intdisplay.h"
#include "action.h"
#include "difficulty.h"
#include "powercrypt.h"


#define EXTRACT_POINTS	    1
#define EASY_POWER_MOD      110
#define NORMAL_POWER_MOD    100
#define HARD_POWER_MOD      90

//arbitary high value - needs to allow all structures to be built at start of any game
#define MAX_POWER	100000

//flag used to check for power calculations to be done or not
BOOL	powerCalculated;

/*Update the capcity and available power if necessary */
//static void availablePowerUpdate(STRUCTURE *psBuilding);
/*looks through the player's list of droids and structures to see what power
has been used*/
//static UDWORD calcPlayerUsedPower(UDWORD player);

/* Updates the current power based on the extracted power and a Power Generator*/
static void updateCurrentPower(POWER_GEN *psPowerGen, UDWORD player);

/*accrue the power in the facilities that require it*/
//static void accruePower(STRUCTURE *psStructure);

//returns the relevant list based on OffWorld or OnWorld
static STRUCTURE* powerStructList(UBYTE player);
//returns the relevant list based on OffWorld or OnWorld for the accruePower function
//static STRUCTURE* powerUpdateStructList(UBYTE player);

PLAYER_POWER		*asPower[MAX_PLAYERS];

/*allocate the space for the playerPower*/
BOOL allocPlayerPower(void)
{
	UDWORD		player;

	//allocate the space for the structure
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		asPower[player] = (PLAYER_POWER *) MALLOC (sizeof(PLAYER_POWER));
		if (asPower[player] == NULL)
		{
			debug( LOG_ERROR, "Out of memory" );
			abort();
			return FALSE;
		}
	}
	clearPlayerPower();
	powerCalculated = TRUE;
	return TRUE;
}

/*clear the playerPower */
void clearPlayerPower(void)
{
	UDWORD player;

	//check power has been allocated!
	if (asPower[0] == NULL)
	{
		return;
	}
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		memset(asPower[player], 0, sizeof(PLAYER_POWER));
		pwrcSetPlayerCryptPower(player,0);
	}
}

/*Free the space used for playerPower */
void releasePlayerPower(void)
{
	UDWORD player;

	//check power has been allocated!
	if (asPower[0] == NULL)
	{
		return;
	}
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if (asPower[player])
		{
			FREE(asPower[player]);
		}
	}
}

/*check the current power - if enough return true, else return false */
BOOL checkPower(UDWORD player, UDWORD quantity, BOOL playAudio)
{
	//if not doing a check on the power - just return TRUE
	if (!powerCalculated)
	{
		return TRUE;
	}

	if (!pwrcCheckPlayerCryptPower(player, asPower[player]->currentPower))
	{
		return FALSE;
	}

	if (asPower[player]->currentPower >= quantity)
	{
		return TRUE;
	}
    //Not playing the power low message anymore - 6/1/99
	/*else if (player == selectedPlayer)
	{
//#ifdef PSX
//#warning POWER LOW IS DISABLED
//		return TRUE;
//#endif
		if (playAudio && player == selectedPlayer)
		{
			audio_QueueTrack( ID_SOUND_POWER_LOW );
			return FALSE;
		}
	}*/
	return FALSE;
}

/*check the current power - if enough subtracts the amount
 required to perform the task and returns true, else returns
 false */
BOOL usePower(UDWORD player, UDWORD quantity)
{
	//if not doing a check on the power - just return TRUE
	if (!powerCalculated)
	{
		return TRUE;
	}

	if (!pwrcCheckPlayerCryptPower(player, asPower[player]->currentPower))
	{
		return FALSE;
	}

	//check there is enough first
	if (asPower[player]->currentPower >= quantity)
	{
		asPower[player]->currentPower -= quantity;
		pwrcSetPlayerCryptPower(player, asPower[player]->currentPower);
		return TRUE;
	}
	else if (player == selectedPlayer)
	{
//#ifdef PSX
//#warning POWER LOW IS DISABLED
//		return TRUE;
//#endif

		if(titleMode == FORCESELECT) //|| (titleMode == DESIGNSCREEN))
		{
			return FALSE;
		}

        //Not playing the power low message anymore - 6/1/99
		//audio_QueueTrack( ID_SOUND_POWER_LOW );
	}
	return FALSE;
}

//return the power when a structure/droid is deliberately destroyed
void addPower(UDWORD player, UDWORD quantity)
{
	if (!pwrcCheckPlayerCryptPower(player, asPower[player]->currentPower))
	{
		return;
	}

	asPower[player]->currentPower += quantity;
	pwrcSetPlayerCryptPower(player, asPower[player]->currentPower);
}

/*resets the power calc flag for all players*/
void powerCalc(BOOL on)
{
	if (on)
	{
		powerCalculated = TRUE;
	}
	else
	{
		powerCalculated = FALSE;
	}
}

/* Each Resource Extractor yields EXTRACT_POINTS per second until there are none
   left in the resource. */
UDWORD updateExtractedPower(STRUCTURE	*psBuilding)
{
	RES_EXTRACTOR		*pResExtractor;
	UDWORD				pointsToAdd, extractedPoints, timeDiff;
    UBYTE               modifier;

	pResExtractor = (RES_EXTRACTOR *) psBuilding->pFunctionality;
	extractedPoints = 0;

	//only extracts points whilst its active ie associated with a power gen
	//and has got some power to extract
	if (pResExtractor->active AND pResExtractor->power)
	{
        timeDiff = gameTime - pResExtractor->timeLastUpdated;
        //add modifier according to difficulty level
        if (getDifficultyLevel() == DL_EASY)
        {
            modifier = EASY_POWER_MOD;
        }
        else if (getDifficultyLevel() == DL_HARD)
        {
            modifier = HARD_POWER_MOD;
        }
        else
        {
            modifier = NORMAL_POWER_MOD;
        }
        //include modifier as a %
		pointsToAdd = (modifier * EXTRACT_POINTS * timeDiff) / (GAME_TICKS_PER_SEC * 100);
		if (pointsToAdd)
		{
            //lose a lot on rounding this way
			pResExtractor->timeLastUpdated = gameTime;
            //pResExtractor->timeLastUpdated = gameTime - (timeDiff - GAME_TICKS_PER_SEC);
			if (pResExtractor->power > pointsToAdd)
			{
				extractedPoints += pointsToAdd;
				pResExtractor->power -= pointsToAdd;
			}
			else
			{
				extractedPoints += pResExtractor->power;
				pResExtractor->power = 0;
			}

			if (pResExtractor->power == 0)
			{
                //if not having unlimited power, put the 2 lines below back in
				//set the extractor to be inactive
				//pResExtractor->active = FALSE;
				//break the link between the power gen and the res extractor
				//releaseResExtractor(psBuilding);

				//for now, when the power = 0 set it back to the max level!
				pResExtractor->power = ((RESOURCE_FUNCTION*)psBuilding->pStructureType->
					asFuncList[0])->maxPower;
			}
		}
	}
	return extractedPoints;
}

/* Each Resource Extractor yields EXTRACT_POINTS per second until there are none
   left in the resource. */
/*void updateExtractedPower(STRUCTURE	*psBuilding)
{
	RES_EXTRACTOR		*pResExtractor;
	UDWORD				pointsToAdd;

	pResExtractor = (RES_EXTRACTOR *) psBuilding->pFunctionality;

	//only extracts points whilst its active ie associated with a power gen
	if (pResExtractor->active)
	{
		pointsToAdd = EXTRACT_POINTS * (gameTime - pResExtractor->timeLastUpdated) /
			GAME_TICKS_PER_SEC;

		if (pointsToAdd)
		{
			pResExtractor->timeLastUpdated = gameTime;
			if (pResExtractor->power > pointsToAdd)
			{
				asPower[psBuilding->player]->extractedPower += pointsToAdd;
				pResExtractor->power -= pointsToAdd;
			}
			else
			{
				asPower[psBuilding->player]->extractedPower += pResExtractor->power;
				pResExtractor->power = 0;
			}

			//for now, when the power = 0 set it back to the max level!
			if (pResExtractor->power == 0)
			{
				pResExtractor->power = ((POWER_REG_FUNCTION*)psBuilding->pStructureType->
					asFuncList[0])->maxPower;
			}
		}
	}
}*/

//returns the relevant list based on OffWorld or OnWorld
STRUCTURE* powerStructList(UBYTE player)
{
	if (offWorldKeepLists)
	{
		return (mission.apsStructLists[player]);
	}
	else
	{
		return (apsStructLists[player]);
	}
}

//returns the relevant list based on OffWorld or OnWorld for the accruePower function
/*STRUCTURE* powerUpdateStructList(UBYTE player)
{
	static BYTE		first[MAX_PLAYERS] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};

	if (offWorldKeepLists)
	{
		if (first[player])
		{
			first[player] = FALSE;
			return (mission.apsStructLists[player]);
		}
		else
		{
			first[player] = TRUE;
			return (apsStructLists[player]);
		}
	}
	else
	{
		return (apsStructLists[player]);
	}
}*/

/* Update current power based on what Power Generators exist */
void updatePlayerPower(UDWORD player)
{
	STRUCTURE		*psStruct;//, *psList;

	/*if (offWorldKeepLists)
	{
		psList = mission.apsStructLists[player];
	}
	else
	{
		psList = apsStructLists[player];
	}*/

	//for (psStruct = psList; psStruct != NULL; psStruct = psStruct->psNext)
	for (psStruct = powerStructList((UBYTE)player); psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_POWER_GEN AND psStruct->
			status == SS_BUILT)
		{
			updateCurrentPower((POWER_GEN *)psStruct->pFunctionality, player);
		}
	}
    //check that the psLastPowered hasn't died
    if (asPower[player]->psLastPowered AND asPower[player]->psLastPowered->died)
    {
        asPower[player]->psLastPowered = NULL;
    }

}

/* Update current power based on what was extracted during the last cycle and
   what Power Generators exist */
/*void updatePlayerPower(UDWORD player)
{
	STRUCTURE		*psStruct;

	if (asPower[player]->extractedPower == 0)
	{
		return;
	}

	//may need to order the structures so that the Power Gen with the highest
	//multiplier is used first so that the player gets maximum power output. For now
	//all multiplier are the same

	for (psStruct = apsStructLists[player]; psStruct != NULL AND
		asPower[player]->extractedPower != 0; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_POWER_GEN AND psStruct->
			status == SS_BUILT)
		{
			updateCurrentPower((POWER_GEN *)psStruct->pFunctionality, player);
		}
	}
}*/

/* Updates the current power based on the extracted power and a Power Generator*/
void updateCurrentPower(POWER_GEN *psPowerGen, UDWORD player)
{
	UDWORD		power, i, extractedPower;

	if (!pwrcCheckPlayerCryptPower(player, asPower[player]->currentPower))
	{
		return;
	}

	//each power gen can cope with its associated resource extractors
	extractedPower = 0;
	//for (i=0; i < (NUM_POWER_MODULES + 1); i++)
	//each Power Gen can cope with 4 extractors now - 9/6/98 AB
	for (i=0; i < NUM_POWER_MODULES; i++)
	{
		if (psPowerGen->apResExtractors[i])
		{
			//check not died
			if (psPowerGen->apResExtractors[i]->died)
			{
				psPowerGen->apResExtractors[i] = NULL;
			}
			else
			{
				extractedPower += updateExtractedPower(psPowerGen->apResExtractors[i]);
			}
		}
	}

	asPower[player]->extractedPower += extractedPower ;
	power = (asPower[player]->extractedPower * psPowerGen->multiplier) / 100;
	if (power)
	{
		asPower[player]->currentPower += power;
		asPower[player]->extractedPower = 0;

		pwrcSetPlayerCryptPower(player, asPower[player]->currentPower);
	}
}

/* Updates the current power based on the extracted power and a Power Generator*/
/*void updateCurrentPower(POWER_GEN *psPowerGen, UDWORD player)
{
	UDWORD		power;

	//each power gen can cope with a set amount of power
	power = (asPower[player]->extractedPower * psPowerGen->multiplier) / 100;
	if (power)
	{
		if (power > psPowerGen->power)
		{
			power = psPowerGen->power;
			asPower[player]->extractedPower -= ((power / psPowerGen->multiplier)/100);
		}
		else
		{
			//no power left over
			asPower[player]->extractedPower = 0;
		}
		if (power)
		{
			asPower[player]->currentPower += power;
		}
	}
}*/


// only used in multiplayer games.
void setPower(UDWORD player, UDWORD avail)
{
	if (!pwrcCheckPlayerCryptPower(player, asPower[player]->currentPower))
	{
		return;
	}

	asPower[player]->currentPower = avail;

	pwrcSetPlayerCryptPower(player, asPower[player]->currentPower);
}



/*sets the initial value for the power*/
void setPlayerPower(UDWORD power, UDWORD player)
{
	if (!pwrcCheckPlayerCryptPower(player, asPower[player]->currentPower))
	{
		return;
	}

	//asPower[player]->initialPower = power;
	asPower[player]->currentPower = power;

	pwrcSetPlayerCryptPower(player, asPower[player]->currentPower);
}

/*Temp function to give all players some power when a new game has been loaded*/
void newGameInitPower(void)
{
	UDWORD		inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//setPlayerPower(400, inc);
        //add as opposed to set
        addPower(inc, 400);
	}
}

/*this keeps track of which object was the last to receive power. Power is
passed through the object lists each cycle whilst there is some*/
/*void spreadPower(UBYTE player)
{
	STRUCTURE	*psLastPowered = NULL, *psStartStruct;
	UDWORD		lastPower = asPower[player]->currentPower;
	UBYTE		first = TRUE, warning = 0;

	psStartStruct = asPower[player]->psLastPowered;
	//if (!psStartStruct OR psStartStruct->died)
	//if (!psStartStruct)
	//{
	//	psStartStruct = powerUpdateStructList(player);
	//}

	//got to have the minimum power
	while ((asPower[player]->currentPower > POWER_PER_CYCLE) OR (!powerCalculated))
	{
		//little test to see if we're looping indefinately!
		warning++;
		if (warning > 1000)
		{
			ASSERT( FALSE,
				"spreading power round more than 1000 buildings for player %d?",
				player );
			warning = 0;
		}

		//if haven't used any power and been through the whole list of
		//structures then jump out
		if (!first)
		{
			if (psStartStruct == asPower[player]->psLastPowered)
			{
				//back to the start struct
				if (lastPower == asPower[player]->currentPower)
				{
					return;
				}
				lastPower = asPower[player]->currentPower;
			}
		}
		first = FALSE;
		//determine which object is to receive the power
		//check for last in list or nothing available
		if (asPower[player]->psLastPowered == NULL)
		{
			//back to top of list
			psNextPowered = powerUpdateStructList(player);
			psStartStruct = psNextPowered;
			warning = 0;
		}
		else
		{
			//get the next in the list
			psNextPowered = asPower[player]->psLastPowered->psNext;
			//check for last in list
			if (psNextPowered == NULL)
			{
				psNextPowered = powerUpdateStructList(player);
				warning = 0;
			}
		}
		if (psNextPowered)
		{
			accruePower(psNextPowered);
			asPower[player]->psLastPowered = psNextPowered;
		}
	}
}*/

/*accrue the power in the facilities that require it - returns TRUE if use some power*/
BOOL accruePower(BASE_OBJECT *psObject)
{
	FACTORY					*psFactory;
	RESEARCH_FACILITY		*psResearch;
	REPAIR_FACILITY			*psRepair;
	SDWORD					powerDiff;
	UDWORD					count;
	BOOL					bPowerUsed = FALSE;
    STRUCTURE               *psStructure;
    DROID                   *psDroid, *psTarget;

    switch(psObject->type)
    {
    case OBJ_STRUCTURE:
        psStructure = (STRUCTURE *)psObject;
	    //see if it needs power
	    switch(psStructure->pStructureType->type)
    	{
	    case REF_FACTORY:
	    case REF_CYBORG_FACTORY:
    	case REF_VTOL_FACTORY:
	    	psFactory = (FACTORY *)psStructure->pFunctionality;
		    //check the factory is not on hold
            if (psFactory->timeStartHold)
            {
                break;
            }
		    //check the factory is active
		    if (psFactory->psSubject)
		    {
			    //check needs power
			    powerDiff = ((DROID_TEMPLATE *)psFactory->psSubject)->powerPoints -
				    psFactory->powerAccrued;
			    //if equal then don't need power
			    if (powerDiff)
			    {
				    if (POWER_PER_CYCLE >= powerDiff)
				    {
					    usePower(psStructure->player, powerDiff);
					    psFactory->powerAccrued += powerDiff;
					    bPowerUsed = TRUE;
				    }
				    else if (powerDiff > POWER_PER_CYCLE)
				    {
					    usePower(psStructure->player, POWER_PER_CYCLE);
					    psFactory->powerAccrued += POWER_PER_CYCLE;
					    bPowerUsed = TRUE;
				    }
			    }
    		}
	    	break;
	    case REF_RESEARCH:
		    //check the structure is active
		    psResearch = (RESEARCH_FACILITY  *)psStructure->pFunctionality;

		    //check the research facility is not on hold
            if (psResearch->timeStartHold)
            {
                break;
            }
		    if (psResearch->psSubject)
		    {
			    //check the research hasn't been cancelled
			    count = ((RESEARCH *)psResearch->psSubject)->ref - REF_RESEARCH_START;
			    if (IsResearchCancelled(asPlayerResList[selectedPlayer] + count)==FALSE)
			    {
				    //check needs power
				    powerDiff = ((RESEARCH *)psResearch->psSubject)->researchPower -
					    psResearch->powerAccrued;
				    //if equal then don't need power
				    if (powerDiff)
				    {
					    //use the power if appropriate
					    if (POWER_PER_CYCLE >= powerDiff)
					    {
						    usePower(psStructure->player, powerDiff);
						    psResearch->powerAccrued += powerDiff;
						    bPowerUsed = TRUE;
					    }
					    else if (powerDiff > POWER_PER_CYCLE)
					    {
						    usePower(psStructure->player, POWER_PER_CYCLE);
						    psResearch->powerAccrued += POWER_PER_CYCLE;
						    bPowerUsed = TRUE;
					    }
				    }
			    }
		    }
		    break;
	    case REF_REPAIR_FACILITY:
		    //POWER REQUIRMENTS REMOVED - AB  22/09/98 - BACK IN - AB 07/01/99
		    psRepair = (REPAIR_FACILITY *)psStructure->pFunctionality;
            psDroid = (DROID *)psRepair->psObj;
            //check the droid hasn't died in the meantime
            if (psRepair->psObj AND psRepair->psObj->died)
            {
                psRepair->psObj = NULL;
            }
		    if (psRepair->psObj)
		    {
			    //check if need power
                powerDiff = powerReqForDroidRepair(psDroid) - psDroid->powerAccrued;
                //if equal then don't need power
			    if (powerDiff > 0)
			    {
                    powerDiff /= POWER_FACTOR;
				    if (POWER_PER_CYCLE >= powerDiff)
				    {
					    usePower(psStructure->player, powerDiff);
                        //the unit accrues the power so more than one thing can be working on it
					    psDroid->powerAccrued += (powerDiff * POWER_FACTOR);
					    bPowerUsed = TRUE;
				    }
				    else if (powerDiff > POWER_PER_CYCLE)
				    {
					    usePower(psStructure->player, POWER_PER_CYCLE);
					    psDroid->powerAccrued += (POWER_PER_CYCLE * POWER_FACTOR);
					    bPowerUsed = TRUE;
				    }
			    }
		    }
		    break;
	    default:
		    //no need for power
		    bPowerUsed = FALSE;
		    break;
        }
        break;
    case OBJ_DROID:
        psDroid = (DROID *)psObject;
        switch (psDroid->droidType)
        {
        case DROID_CONSTRUCT:
        case DROID_CYBORG_CONSTRUCT:
            //check trying to build something (and that hasn't been blown up)
            if (DroidIsBuilding(psDroid) AND psDroid->psTarget AND !psDroid->psTarget->died)
            {
			    //powerDiff = ((STRUCTURE *)psDroid->psTarget)->pStructureType->
                //    powerToBuild - ((STRUCTURE *)psDroid->psTarget)->
                //    currentPowerAccrued;
                powerDiff = structPowerToBuild((STRUCTURE *)psDroid->psTarget) -
                    ((STRUCTURE *)psDroid->psTarget)->currentPowerAccrued;
			    //if equal then don't need power
			    if (powerDiff)
			    {
				    if (POWER_PER_CYCLE >= powerDiff)
				    {
					    usePower(psDroid->player, powerDiff);
					    ((STRUCTURE *)psDroid->psTarget)->currentPowerAccrued +=
                            powerDiff;
					    bPowerUsed = TRUE;
				    }
				    else if (powerDiff > POWER_PER_CYCLE)
				    {
					    usePower(psDroid->player, POWER_PER_CYCLE);
					    ((STRUCTURE *)psDroid->psTarget)->currentPowerAccrued +=
                            POWER_PER_CYCLE;
					    bPowerUsed = TRUE;
				    }
			    }
            }
            break;
        case DROID_REPAIR:
        case DROID_CYBORG_REPAIR:
            //check trying to repair something
            psTarget = NULL;
            if (DroidIsRepairing(psDroid))
            {
                psTarget = (DROID *)psDroid->psTarget;
            }
            else
            {
                //might have guard order but action of repair
                if (orderState(psDroid, DORDER_GUARD) AND psDroid->action ==
                    DACTION_DROIDREPAIR)
                {
                    psTarget = (DROID *)psDroid->psActionTarget;
                }
            }
            //check the droid hasn't died in the meantime
            if (psTarget AND psTarget->died)
            {
                psDroid->psTarget = NULL;
                psTarget = NULL;
            }
            if (psTarget)
            {
                powerDiff = powerReqForDroidRepair(psTarget) - psTarget->powerAccrued;
			    //if equal then don't need power
			    if (powerDiff > 0)
			    {
                    powerDiff /= POWER_FACTOR;
				    if (POWER_PER_CYCLE >= powerDiff)
				    {
					    usePower(psDroid->player, powerDiff);
                        //the unit accrues the power so more than one thing can be working on it
					    psTarget->powerAccrued += (powerDiff * POWER_FACTOR);
					    bPowerUsed = TRUE;
				    }
				    else if (powerDiff > POWER_PER_CYCLE)
				    {
					    usePower(psDroid->player, POWER_PER_CYCLE);
					    psTarget->powerAccrued += (POWER_PER_CYCLE * POWER_FACTOR);
					    bPowerUsed = TRUE;
				    }
			    }
            }
            break;
        default:
		    //no need for power
		    bPowerUsed = FALSE;
		    break;
        }
        break;
    default:
        ASSERT( FALSE, "accruePower: Invalid object type" );
    }

	return bPowerUsed;
}


//informs the power array that a object has been destroyed
void powerDestroyObject(BASE_OBJECT *psObject)
{
	//check that this wasn't the last object that received the power
	if (asPower[psObject->player]->psLastPowered == psObject)
	{
		updateLastPowered(NULL, psObject->player);
	}
}

/*checks if the Object to be powered next - returns TRUE if power*/
BOOL getLastPowered(BASE_OBJECT *psObject)
{
	ASSERT( psObject != NULL, "getLastPowered - invalid object" );

	if (asPower[psObject->player]->psLastPowered == NULL)
	{
		return TRUE;
	}
	/*if we've got round to the last object again, by setting to NULL will
	enable the next object to get some power*/
	if (asPower[psObject->player]->psLastPowered == psObject)
	{
		asPower[psObject->player]->psLastPowered = NULL;
	}
	return FALSE;
}

/*inform the players power struct that the last object to receive power has changed*/
void updateLastPowered(BASE_OBJECT *psObject, UBYTE player)
{
	asPower[player]->psLastPowered = psObject;
}

STRUCTURE *getRExtractor(STRUCTURE *psStruct)
{
STRUCTURE	*psCurr;
STRUCTURE	*psFirst;
BOOL		bGonePastIt;

	for(psCurr = apsStructLists[selectedPlayer],psFirst = NULL,bGonePastIt = FALSE;
		psCurr; psCurr = psCurr->psNext)
	{
		if( psCurr->pStructureType->type == REF_RESOURCE_EXTRACTOR )
		{

			if(!psFirst)
			{
				psFirst = psCurr;
			}

			if(!psStruct)
			{
				return(psCurr);
			}
			else if(psCurr!=psStruct AND bGonePastIt)
			{
				return(psCurr);
			}

			if(psCurr==psStruct)
			{
				bGonePastIt = TRUE;
			}


		}
	}
	return(psFirst);
}

/*defines which structure types draw power - returns TRUE if use power*/
BOOL structUsesPower(STRUCTURE *psStruct)
{
    BOOL    bUsesPower = FALSE;

	ASSERT( PTRVALID(psStruct, sizeof(STRUCTURE)),
		"structUsesPower: Invalid Structure pointer" );

    switch(psStruct->pStructureType->type)
    {
        case REF_FACTORY:
	    case REF_CYBORG_FACTORY:
    	case REF_VTOL_FACTORY:
	    case REF_RESEARCH:
	    case REF_REPAIR_FACILITY:
            bUsesPower = TRUE;
            break;
        default:
            bUsesPower = FALSE;
            break;
    }

    return bUsesPower;
}

/*defines which droid types draw power - returns TRUE if use power*/
BOOL droidUsesPower(DROID *psDroid)
{
    BOOL    bUsesPower = FALSE;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"unitUsesPower: Invalid unit pointer" );

    switch(psDroid->droidType)
    {
        case DROID_CONSTRUCT:
	    case DROID_REPAIR:
        case DROID_CYBORG_CONSTRUCT:
        case DROID_CYBORG_REPAIR:
            bUsesPower = TRUE;
            break;
        default:
            bUsesPower = FALSE;
            break;
    }

    return bUsesPower;
}

//won't bother with this on PSX unless starts being used too much!

//this is a check cos there is a problem with the power but not sure where!!
void powerCheck(BOOL bBeforePowerUsed, UBYTE player)
{
    static  BASE_OBJECT     *psLastPowered = NULL;
    static  BOOL            bPowerBefore = FALSE;

    if (bBeforePowerUsed)
    {
        //set what the lastPowered object is before using any power
        psLastPowered = asPower[player]->psLastPowered;
        bPowerBefore = FALSE;
        //check that there is power available at start of loop
        if (asPower[player]->currentPower > POWER_PER_CYCLE)
        {
            bPowerBefore = TRUE;
        }

    }
    else
    {
        /*check to see if we've been thru the whole list of structures and
        droids and not reset the lastPowered object in the power structure and
        there was some power at the start of the loop to use*/
        if (psLastPowered != NULL AND psLastPowered == asPower[player]->
            psLastPowered AND bPowerBefore)
        {
            ASSERT( FALSE, "powerCheck: trouble at mill!" );
            //initialise so something can have some power next cycle
            asPower[player]->psLastPowered = NULL;
        }
    }
}



/*initialise the PlayerPower based on what structures are available*/
/*BOOL initPlayerPower(void)
{
	UDWORD		player, usedPower;
	STRUCTURE	*psBuilding;

	//total up the resources
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		asPower[player]->extractedPower = 0;
		for (psBuilding = apsStructLists[player]; psBuilding != NULL; psBuilding =
			psBuilding->psNext)
		{
			if (psBuilding->pStructureType->type == REF_RESOURCE_EXTRACTOR OR
				psBuilding->pStructureType->type == REF_HQ)
			{
				extractedPowerUpdate(psBuilding);
			}
		}
	}

	//calculate the total available and capacity
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		asPower[player]->availablePower = 0;
		asPower[player]->capacity = 0;
		for (psBuilding = apsStructLists[player]; psBuilding != NULL; psBuilding =
			psBuilding->psNext)
		{
			if (psBuilding->pStructureType->type == REF_POWER_GEN OR
				psBuilding->pStructureType->type == REF_HQ)
			{
				capacityUpdate(psBuilding);
			}
		}
	}

	//adjust for the power already used
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		usedPower = calcPlayerUsedPower(player);
		if (usedPower > 0)
		{
			if ((SDWORD)asPower[player]->availablePower < usedPower)
			{
				asPower[player]->usedPower = asPower[player]->availablePower -
					usedPower;
				asPower[player]->availablePower = 0;
//				audio_PlayTrack( ID_SOUND_POWER_LOW );
			}
			else
			{
				asPower[player]->availablePower -= usedPower;
				asPower[player]->usedPower = usedPower;
			}
		}
	}

	return TRUE;
}*/


/*check the available power - if enough return true, else return false */
/*BOOL checkPower(UDWORD player, UDWORD quantity, BOOL playAudio)
{
	//if not doing a check on the power - just return TRUE
	if (!powerCalculated)
	{
		return TRUE;
	}

	if (asPower[player]->availablePower >= quantity)
	{
		return TRUE;
	}
	else if (player == selectedPlayer)
	{
#ifdef PSX
#warning POWER LOW IS DISABLED
		return TRUE;
#endif
		if (playAudio && player == selectedPlayer)
		{
			audio_QueueTrack( ID_SOUND_POWER_LOW );
			return FALSE;
		}
	}
	return FALSE;
}*/

/*check the available power - if enough subtracts the amount
 required to perform the task and returns true, else returns
 false */
/*BOOL usePower(UDWORD player, UDWORD quantity)
{
	//if not doing a check on the power - just use the power and return TRUE
	if (!powerCalculated)
	{
		if (asPower[player]->usedPower >= 0)
		{
			asPower[player]->usedPower += quantity;
		}
		else
		{
			asPower[player]->usedPower -= quantity;
		}

		return TRUE;
	}

	//check there is enough first
	if (asPower[player]->availablePower >= quantity)
	{
		asPower[player]->availablePower -= quantity;
		asPower[player]->usedPower += quantity;
		return TRUE;
	}
	else if (player == selectedPlayer)
	{
#ifdef PSX
#warning POWER LOW IS DISABLED
		return TRUE;
#endif

		if(titleMode == FORCESELECT) //|| (titleMode == DESIGNSCREEN))
		{
			return FALSE;
		}

		if(player == selectedPlayer)
		{
			audio_QueueTrack( ID_SOUND_POWER_LOW );
		}

		return FALSE;
	}
}*/

//return the power when a structure/droid is destroyed
/*void returnPower(UDWORD player, UDWORD quantity)
{
	if (asPower[player]->usedPower < 0)
	{
		asPower[player]->usedPower += quantity;
	}
	else
	{
		asPower[player]->usedPower -= quantity;
		asPower[player]->availablePower += quantity;
	}
}*/

/*Update the available power if necessary */
/*void availablePowerUpdate(STRUCTURE *psBuilding)
{
	UDWORD			power;

	//check its the correct type of building
	if (psBuilding->pStructureType->type == REF_POWER_GEN OR
		psBuilding->pStructureType->type == REF_HQ)
	{
		if ((SDWORD)(asPower[psBuilding->player]->extractedPower) > asPower[psBuilding->
			player]->capacity)
		{
			power = asPower[psBuilding->player]->capacity;
			asPower[psBuilding->player]->extractedPower -= power;
		}
		else
		{
			power = asPower[psBuilding->player]->extractedPower;
			asPower[psBuilding->player]->extractedPower = 0;
		}

		//for power gens need to use the multiplier
		//if (psBuilding->pStructureType->type == REF_POWER_GEN)
		//{
		//	power = power * ((POWER_GEN *)psBuilding->pFunctionality)->multiplier;
		//}

		asPower[psBuilding->player]->capacity -= power;
		asPower[psBuilding->player]->availablePower += power ;
	}
}*/

/*Update the extracted power if necessary */
/*void extractedPowerUpdate(STRUCTURE *psBuilding)
{
	STRUCTURE	*psStruct;
	UDWORD		power = 0;

	if (psBuilding->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		power = ((RES_EXTRACTOR *)psBuilding->pFunctionality)->power;
	}
	if (psBuilding->pStructureType->type == REF_HQ)
	{
		power = ((HQ_FUNCTION *)psBuilding->pStructureType->asFuncList[0])->power;
	}
	if (power)
	{
		asPower[psBuilding->player]->extractedPower += power;

		//increase in extracted power might lead to more power being available.
		if (asPower[psBuilding->player]->capacity != 0)
		{
			for (psStruct = apsStructLists[psBuilding->player]; psStruct != NULL AND
				asPower[psBuilding->player]->extractedPower != 0; psStruct =
				psStruct->psNext)
			{
				if (psStruct->pStructureType->type == REF_POWER_GEN OR
					psStruct->pStructureType->type == REF_HQ)
				{
					availablePowerUpdate(psStruct);
				}
			}
		}
	}
}*/

/*update the generator capacity if necessary. This function is called whenever
  a Power Generator or Power Module is built or HQ is built*/
/*void capacityUpdate(STRUCTURE * psBuilding)
{
	UDWORD		power = 0;

	if (psBuilding->pStructureType->type == REF_POWER_GEN)
	{
		//only add on module power if this is what has been built BIT OF A HACK REALLY!
		if (((POWER_GEN*)psBuilding->pFunctionality)->power != ((POWER_GEN_FUNCTION*)
			psBuilding->pStructureType->asFuncList[0])->powerOutput)
		{
			//subtract the power output without modules
			asPower[psBuilding->player]->capacity -= ((POWER_GEN_FUNCTION*)
				psBuilding->pStructureType->asFuncList[0])->powerOutput;
		}
		//asPower[psBuilding->player]->capacity += (((POWER_GEN *)psBuilding->
		//	pFunctionality)->power * ((POWER_GEN *)psBuilding->pFunctionality)->
		//	multiplier);

		power = (((POWER_GEN *)psBuilding->pFunctionality)->power *
			((POWER_GEN *)psBuilding->pFunctionality)->multiplier);
	}
	else
	{
		if (psBuilding->pStructureType->type == REF_HQ)
		{
			power = ((HQ_FUNCTION *)psBuilding->pStructureType->asFuncList[0])->power;
		}
	}
	if (power)
	{
		asPower[psBuilding->player]->capacity += power;

		//now check if there is any spare extracted power
		if (asPower[psBuilding->player]->extractedPower != 0)
		{
			availablePowerUpdate(psBuilding);
		}
	}
}*/

/*reset the power levels when a power_gen or resource_extractor is destroyed */
/*BOOL resetPlayerPower(UDWORD player, STRUCTURE *psStruct)
{
	STRUCTURE	*psBuilding;
	UDWORD		usedPower;


	asPower[player]->extractedPower = 0;
	asPower[player]->capacity = 0;

	//total up the resources
	for (psBuilding = apsStructLists[player]; psBuilding != NULL; psBuilding =
		psBuilding->psNext)
	{
		if (psStruct != psBuilding)
		{
			if (psBuilding->pStructureType->type == REF_RESOURCE_EXTRACTOR OR
				psBuilding->pStructureType->type == REF_HQ)
			{
				extractedPowerUpdate(psBuilding);
			}
		}
	}

	asPower[player]->availablePower = 0;

	// add multiplayer allowances if a multiplayer game.
#ifndef PSX
	if(bMultiPlayer && (game.dmatch || game.base == CAMP_CLEAN))
	{
		asPower[player]->availablePower = game.power+RESIDUAL_POW;
	}
#endif

	//calculate the total available and capacity
	for (psBuilding = apsStructLists[player]; psBuilding != NULL; psBuilding =
		psBuilding->psNext)
	{
		if (psStruct != psBuilding)
		{
			if (psBuilding->pStructureType->type == REF_POWER_GEN OR
				psBuilding->pStructureType->type == REF_HQ)
			{
				capacityUpdate(psBuilding);
			}
		}
	}

	//adjust for the power already used
	usedPower = calcPlayerUsedPower(player);
	if ((SDWORD)asPower[player]->availablePower < usedPower)
	{
		asPower[player]->usedPower = asPower[player]->availablePower -
			usedPower;
		asPower[player]->availablePower = 0;
		if (player == selectedPlayer)
		{
			audio_QueueTrack( ID_SOUND_POWER_LOW );
		}
	}
	else
	{
		asPower[player]->availablePower -= usedPower;
		asPower[player]->usedPower = usedPower;
	}

	return TRUE;
}*/


/*looks through the player's list of droids and structures to see what power
has been used*/
/*UDWORD calcPlayerUsedPower(UDWORD player)
{
	DROID		*psDroid;
	STRUCTURE	*psStruct;
	UDWORD		used = 0;

	for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->
		psNext)
	{
		used += psDroid->power;
	}
	for (psStruct = apsStructLists[player]; psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		used += psStruct->pStructureType->powerToBuild;
	}
	return used;
}*/

/*resets the power levels for all players when power is turned back on*/
/*void powerCalc(BOOL on)
{
	UDWORD		inc;

	if (on)
	{
		powerCalculated = TRUE;
		for (inc=0; inc < MAX_PLAYERS; inc++)
		{
			resetPlayerPower(inc, NULL);
		}
	}
	else
	{
		powerCalculated = FALSE;
	}
}*/



