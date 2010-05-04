/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
 * @file power.c
 *
 * Store PlayerPower and other power related stuff!
 *
 */
#include <string.h>
#include "objectdef.h"
#include "power.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "objmem.h"
#include "frontend.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multiint.h"

#include "feature.h"
#include "structure.h"
#include "mission.h"
#include "research.h"
#include "intdisplay.h"
#include "action.h"
#include "difficulty.h"
#include "random.h"


#define EXTRACT_POINTS	    1
#define EASY_POWER_MOD      110
#define NORMAL_POWER_MOD    100
#define HARD_POWER_MOD      90
#define MAX_POWER           100000

//flag used to check for power calculations to be done or not
BOOL	powerCalculated;
UDWORD nextPowerSystemUpdate;

/* Updates the current power based on the extracted power and a Power Generator*/
static void updateCurrentPower(POWER_GEN *psPowerGen, UDWORD player);
/** Each Resource Extractor yields EXTRACT_POINTS per second until there are none left in the resource. */
static float updateExtractedPower(STRUCTURE *psBuilding);

//returns the relevant list based on OffWorld or OnWorld
static STRUCTURE* powerStructList(UBYTE player);

PLAYER_POWER		asPower[MAX_PLAYERS];

/*allocate the space for the playerPower*/
BOOL allocPlayerPower(void)
{
	clearPlayerPower();
	powerCalculated = true;
	return true;
}

/*clear the playerPower */
void clearPlayerPower(void)
{
	UDWORD player;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		asPower[player].currentPower = 0;
		asPower[player].powerProduced = 0;
		asPower[player].powerRequested = 0;
		asPower[player].economyThrottle = 1;
	}
	nextPowerSystemUpdate = 0;
}

void throttleEconomy(void)
{
	int player;
	float newThrottle;

	if (gameTime < nextPowerSystemUpdate)
	{
		return;
	}
	nextPowerSystemUpdate = gameTime + 1000;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if (asPower[player].currentPower > asPower[player].powerRequested ||
		    asPower[player].powerRequested <= asPower[player].powerProduced)
		{
			// we have enough power
			newThrottle = 1;
		}
		else
		{
			newThrottle = (asPower[player].powerProduced + asPower[player].currentPower) / asPower[player].powerRequested;
		}
		if (newThrottle <= asPower[player].economyThrottle)
		{
			// quickly slow down
			asPower[player].economyThrottle = newThrottle;
		}
		else if (asPower[player].powerRequested * asPower[player].economyThrottle * 2 < asPower[player].currentPower)
		{
			// slowly speed up
			asPower[player].economyThrottle += 0.02;
		}
		CLIP(asPower[player].economyThrottle, 0, 1);
		asPower[player].powerProduced = 0;
		asPower[player].powerRequested = 0;
	}
}

/*Free the space used for playerPower */
void releasePlayerPower(void)
{
	// nothing now
}

/*check the current power - if enough return true, else return false */
BOOL checkPower(int player, float quantity)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Bad player (%d)", player);

	//if not doing a check on the power - just return true
	if (!powerCalculated)
	{
		return true;
	}

	if (asPower[player].currentPower >= quantity)
	{
		return true;
	}
	return false;
}

void usePower(int player, float quantity)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player (%d)", player);
	asPower[player].currentPower = MAX(0, asPower[player].currentPower - quantity);
}

void addPower(int player, float quantity)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player (%d)", player);
	asPower[player].currentPower = MAX(0, asPower[player].currentPower + quantity);
	if (asPower[player].currentPower > MAX_POWER)
	{
		asPower[player].currentPower = MAX_POWER;
	}
}

/*resets the power calc flag for all players*/
void powerCalc(BOOL on)
{
	if (on)
	{
		powerCalculated = true;
	}
	else
	{
		powerCalculated = false;
	}
}

/** Each Resource Extractor yields EXTRACT_POINTS per second FOREVER */
float updateExtractedPower(STRUCTURE	*psBuilding)
{
	RES_EXTRACTOR		*pResExtractor;
	SDWORD				timeDiff;
	UBYTE			modifier;
	float pointsToAdd,extractedPoints;

	pResExtractor = (RES_EXTRACTOR *) psBuilding->pFunctionality;
	extractedPoints = 0;

	//only extracts points whilst its active ie associated with a power gen
	//and has got some power to extract
	if (pResExtractor->active)
	{
		// if the extractor hasn't been updated recently, now would be a good time.
		if (pResExtractor->timeLastUpdated < 20 && gameTime >= 20)
		{
			pResExtractor->timeLastUpdated = gameTime;
		}
		timeDiff = (int)gameTime - (int)pResExtractor->timeLastUpdated;
		// Add modifier according to difficulty level
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
		// include modifier as a %
		pointsToAdd = ((float)modifier * EXTRACT_POINTS * timeDiff) / (GAME_TICKS_PER_SEC * 100);

		if (timeDiff > GAME_TICKS_PER_SEC || -timeDiff > GAME_TICKS_PER_SEC)
		{
			// extractor is not in the right time zone
			// we have a maximum time skip of less than a second, so this can't be caused by lag
			ASSERT(false, "Oil derrick out of sync.");
			pointsToAdd = 0;
		}

		pResExtractor->timeLastUpdated = gameTime;
		extractedPoints += pointsToAdd;
	}
	ASSERT(extractedPoints >= 0, "extracted negative amount of power");
	return extractedPoints;
}

//returns the relevant list based on OffWorld or OnWorld
STRUCTURE* powerStructList(UBYTE player)
{
	ASSERT(player < MAX_PLAYERS, "powerStructList: Bad player");
	if (offWorldKeepLists)
	{
		return (mission.apsStructLists[player]);
	}
	else
	{
		return (apsStructLists[player]);
	}
}

/* Update current power based on what Power Generators exist */
void updatePlayerPower(UDWORD player)
{
	STRUCTURE		*psStruct;//, *psList;
	float powerBefore = asPower[player].currentPower;

	ASSERT(player < MAX_PLAYERS, "updatePlayerPower: Bad player");

	for (psStruct = powerStructList((UBYTE)player); psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_POWER_GEN && psStruct->
			status == SS_BUILT)
		{
			updateCurrentPower((POWER_GEN *)psStruct->pFunctionality, player);
		}
	}
	asPower[player].powerProduced += asPower[player].currentPower - powerBefore;
}

/* Updates the current power based on the extracted power and a Power Generator*/
void updateCurrentPower(POWER_GEN *psPowerGen, UDWORD player)
{
	int i;
	float extractedPower;

	ASSERT(player < MAX_PLAYERS, "updateCurrentPower: Bad player");

	//each power gen can cope with its associated resource extractors
	extractedPower = 0;
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

	asPower[player].currentPower += (extractedPower * psPowerGen->multiplier) / 100;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
	if (asPower[player].currentPower > MAX_POWER)
	{
		asPower[player].currentPower = MAX_POWER;
	}
}

// only used in multiplayer games.
void setPower(int player, float power)
{
	ASSERT(player < MAX_PLAYERS, "setPower: Bad player (%u)", player);

	asPower[player].currentPower = power;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
}

float getPower(int player)
{
	ASSERT(player < MAX_PLAYERS, "setPower: Bad player (%u)", player);

	return asPower[player].currentPower;
}

/*Temp function to give all players some power when a new game has been loaded*/
void newGameInitPower(void)
{
	UDWORD		inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		addPower(inc, 400);
	}
}

STRUCTURE *getRExtractor(STRUCTURE *psStruct)
{
STRUCTURE	*psCurr;
STRUCTURE	*psFirst;
BOOL		bGonePastIt;

	for(psCurr = apsStructLists[selectedPlayer],psFirst = NULL,bGonePastIt = false;
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
			else if(psCurr!=psStruct && bGonePastIt)
			{
				return(psCurr);
			}

			if(psCurr==psStruct)
			{
				bGonePastIt = true;
			}


		}
	}
	return(psFirst);
}

/*defines which structure types draw power - returns true if use power*/
BOOL structUsesPower(STRUCTURE *psStruct)
{
    BOOL    bUsesPower = false;

	ASSERT( psStruct != NULL,
		"structUsesPower: Invalid Structure pointer" );

    switch(psStruct->pStructureType->type)
    {
        case REF_FACTORY:
	    case REF_CYBORG_FACTORY:
    	case REF_VTOL_FACTORY:
	    case REF_RESEARCH:
	    case REF_REPAIR_FACILITY:
            bUsesPower = true;
            break;
        default:
            bUsesPower = false;
            break;
    }

    return bUsesPower;
}

/*defines which droid types draw power - returns true if use power*/
BOOL droidUsesPower(DROID *psDroid)
{
    BOOL    bUsesPower = false;

	ASSERT(psDroid != NULL,	"droidUsesPower: Invalid unit pointer" );

    switch(psDroid->droidType)
    {
        case DROID_CONSTRUCT:
	    case DROID_REPAIR:
        case DROID_CYBORG_CONSTRUCT:
        case DROID_CYBORG_REPAIR:
            bUsesPower = true;
            break;
        default:
            bUsesPower = false;
            break;
    }

    return bUsesPower;
}

float requestPower(int player, float amount)
{
	// this is the amount that we are willing to give
	float amountConsidered = amount * asPower[player].economyThrottle;

	if (!powerCalculated)
	{
		return amount; // it's all yours
	}

	// keep track on how much energy we could possibly spend
	asPower[player].powerRequested += amount;
	
	if (amountConsidered <= asPower[player].currentPower)
	{
		// you can have it
		asPower[player].currentPower -= amountConsidered;
		return amountConsidered;
	}
	return 0; // no power this frame
}

// Why is there randomity in the power code?
static int randomRound(float val)
{
	int intPart = val;
	float floatPart = val - intPart;
	if (gameRand(100) < floatPart*100)
	{
		return intPart + 1;
	}
	return intPart;
}

int requestPowerFor(int player, float amount, int points)
{
	int pointsConsidered = randomRound(points * asPower[player].economyThrottle);
	// only what it needs for the n amount of points we consider giving
	float amountConsidered;

	if (points == 0 || amount < 0.01 || !powerCalculated)
	{
		return points;
	}

	amountConsidered = amount * (float) pointsConsidered / points;

	// keep track on how much energy we could possibly spend
	asPower[player].powerRequested += amount;
	
	if (amountConsidered <= asPower[player].currentPower)
	{
		// you can have it
		asPower[player].currentPower -= amountConsidered;
		return pointsConsidered;
	}
	return 0; // no power this frame
}

