/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
#define MAX_POWER           1000000

#define FP_ONE ((int64_t)1 << 32)

//flag used to check for power calculations to be done or not
bool	powerCalculated;
static UDWORD nextPowerSystemUpdate;

/* Updates the current power based on the extracted power and a Power Generator*/
static void updateCurrentPower(POWER_GEN *psPowerGen, UDWORD player);
static int64_t updateExtractedPower(STRUCTURE *psBuilding);

//returns the relevant list based on OffWorld or OnWorld
static STRUCTURE* powerStructList(UBYTE player);

struct PLAYER_POWER
{
	// All fields are 32.32 fixed point.
	int64_t currentPower;         ///< The current amount of power avaialble to the player.
	int64_t powerProduced;        ///< Power produced
	int64_t powerRequested;       ///< Power requested
	int64_t economyThrottle;      ///< What fraction of the requested power is actually delivered
};

static PLAYER_POWER asPower[MAX_PLAYERS];

/*allocate the space for the playerPower*/
bool allocPlayerPower(void)
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
		asPower[player].economyThrottle = FP_ONE;
	}
	nextPowerSystemUpdate = 0;
}

static void syncDebugEconomy(unsigned player, char ch)
{
	syncDebug("%c economy%u = %"PRId64",%"PRId64",%"PRId64",%"PRId64"", ch, player, asPower[player].currentPower, asPower[player].economyThrottle, asPower[player].powerProduced, asPower[player].powerRequested);
}

void throttleEconomy(void)
{
	int player;
	int64_t newThrottle;

	if (gameTime < nextPowerSystemUpdate)
	{
		return;
	}
	nextPowerSystemUpdate = gameTime + 1000;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		syncDebugEconomy(player, '<');

		if (asPower[player].currentPower >= asPower[player].powerRequested ||
		    asPower[player].powerRequested <= asPower[player].powerProduced)
		{
			// we have enough power
			newThrottle = FP_ONE;
		}
		else
		{
			newThrottle = (asPower[player].powerProduced + asPower[player].currentPower) / (asPower[player].powerRequested/FP_ONE + 1);
		}
		if (newThrottle <= asPower[player].economyThrottle)
		{
			// quickly slow down
			asPower[player].economyThrottle = newThrottle;
		}
		else if ((asPower[player].powerRequested/FP_ONE + 1) * asPower[player].economyThrottle * 2 < asPower[player].currentPower)
		{
			// slowly speed up
			asPower[player].economyThrottle += FP_ONE/50;
		}
		CLIP(asPower[player].economyThrottle, 0, FP_ONE);
		asPower[player].powerProduced = 0;
		asPower[player].powerRequested = 0;

		syncDebugEconomy(player, '>');
	}
}

/*check the current power - if enough return true, else return false */
bool checkPower(int player, uint32_t quantity)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Bad player (%d)", player);

	//if not doing a check on the power - just return true
	if (!powerCalculated)
	{
		return true;
	}

	return asPower[player].currentPower >= quantity*FP_ONE;
}

void usePower(int player, uint32_t quantity)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player (%d)", player);
	syncDebug("usePower%d %"PRId64"-=%u", player, asPower[player].currentPower, quantity);
	asPower[player].currentPower = MAX(0, asPower[player].currentPower - quantity*FP_ONE);
}

void addPower(int player, int32_t quantity)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player (%d)", player);
	syncDebug("addPower%d %"PRId64"+=%d", player, asPower[player].currentPower, quantity);
	asPower[player].currentPower += quantity*FP_ONE;
	CLIP(asPower[player].currentPower, 0, MAX_POWER*FP_ONE);
}

/*resets the power calc flag for all players*/
void powerCalc(bool on)
{
	powerCalculated = on;
}

/** Each Resource Extractor yields EXTRACT_POINTS per second FOREVER */
static int64_t updateExtractedPower(STRUCTURE *psBuilding)
{
	RES_EXTRACTOR		*pResExtractor;
	int                     modifier = NORMAL_POWER_MOD;
	int64_t                 extractedPoints;

	pResExtractor = (RES_EXTRACTOR *) psBuilding->pFunctionality;
	extractedPoints = 0;

	//only extracts points whilst its active ie associated with a power gen
	//and has got some power to extract
	if (pResExtractor->active)
	{
		// Add modifier according to difficulty level
		if (game.type == CAMPAIGN)  // other types do not make sense
		{
			switch (getDifficultyLevel())
			{
				case DL_EASY: modifier = EASY_POWER_MOD; break;
				case DL_HARD: modifier = HARD_POWER_MOD; break;
				default: break;
			}
		}
		// include modifier as a %
		extractedPoints = modifier * EXTRACT_POINTS * FP_ONE / (100 * GAME_UPDATES_PER_SEC);
		syncDebug("updateExtractedPower%d = %"PRId64"", psBuilding->player, extractedPoints);
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
	int64_t powerBefore = asPower[player].currentPower;

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
	syncDebug("updatePlayerPower%u %"PRId64"->%"PRId64"", player, powerBefore, asPower[player].currentPower);
}

/* Updates the current power based on the extracted power and a Power Generator*/
static void updateCurrentPower(POWER_GEN *psPowerGen, UDWORD player)
{
	int i;
	int64_t extractedPower;

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

	syncDebug("updateCurrentPower%d = %"PRId64",%u", player, extractedPower, psPowerGen->multiplier);

	asPower[player].currentPower += (extractedPower * psPowerGen->multiplier) / 100;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
	if (asPower[player].currentPower > MAX_POWER*FP_ONE)
	{
		asPower[player].currentPower = MAX_POWER*FP_ONE;
	}
}

void setPower(unsigned player, int32_t power)
{
	ASSERT(player < MAX_PLAYERS, "Bad player (%u)", player);

	syncDebug("setPower%d %"PRId64"->%d", player, asPower[player].currentPower, power);
	asPower[player].currentPower = power*FP_ONE;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
}

void setPrecisePower(unsigned player, int64_t power)
{
	ASSERT(player < MAX_PLAYERS, "Bad player (%u)", player);

	syncDebug("setPower%d %"PRId64"->%"PRId64"", player, asPower[player].currentPower, power);
	asPower[player].currentPower = power;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
}

int32_t getPower(unsigned player)
{
	ASSERT(player < MAX_PLAYERS, "Bad player (%u)", player);

	return asPower[player].currentPower >> 32;
}

int64_t getPrecisePower(unsigned player)
{
	ASSERT(player < MAX_PLAYERS, "Bad player (%u)", player);

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

/*defines which structure types draw power - returns true if use power*/
bool structUsesPower(STRUCTURE *psStruct)
{
    bool    bUsesPower = false;

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
bool droidUsesPower(DROID *psDroid)
{
    bool    bUsesPower = false;

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

// Why is there randomity in the power code?
static int randomRound(int64_t val)
{
	return (val + gameRandU32()) >> 32;
}

int requestPowerFor(int player, int32_t amount, int points)
{
	return requestPrecisePowerFor(player, amount*FP_ONE, points);
}

int requestPrecisePowerFor(int player, int64_t amount, int points)
{
	int pointsConsidered = randomRound(points * asPower[player].economyThrottle);
	// only what it needs for the n amount of points we consider giving
	int64_t amountConsidered;

	if (points == 0 || amount <= 0 || !powerCalculated)
	{
		return points;
	}

	amountConsidered = amount * pointsConsidered / points;

	// keep track on how much energy we could possibly spend
	asPower[player].powerRequested += amount;
	
	if (amountConsidered <= asPower[player].currentPower)
	{
		// you can have it
		asPower[player].currentPower -= amountConsidered;
		syncDebug("requestPrecisePowerFor%d give%d,want%d", player, pointsConsidered, points);
		return pointsConsidered;
	}
	syncDebug("requestPrecisePowerFor%d giveNone,want%d", player, points);
	return 0; // no power this frame
}
