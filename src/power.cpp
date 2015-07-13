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


#define EXTRACT_POINTS      1
#define MAX_POWER           1000000

#define FP_ONE ((int64_t)1 << 32)

//flag used to check for power calculations to be done or not
bool	powerCalculated;

/* Updates the current power based on the extracted power and a Power Generator*/
static void updateCurrentPower(STRUCTURE *psStruct, UDWORD player, int ticks);
static int64_t updateExtractedPower(STRUCTURE *psBuilding);

//returns the relevant list based on OffWorld or OnWorld
static STRUCTURE *powerStructList(int player);

struct PowerRequest
{
	int64_t  amount;              ///< Amount of power being requested.
	unsigned id;                  ///< Structure which is requesting power.
};

struct PlayerPower
{
	// All fields are 32.32 fixed point.
	int64_t currentPower;                  ///< The current amount of power available to the player.
	std::vector<PowerRequest> powerQueue;  ///< Requested power.
	int powerModifier;                 ///< Percentage modifier on power from each derrick.
};

static PlayerPower asPower[MAX_PLAYERS];

void setPowerModifier(int player, int modifier)
{
	asPower[player].powerModifier = modifier;
}

/*allocate the space for the playerPower*/
bool allocPlayerPower()
{
	clearPlayerPower();
	powerCalculated = true;
	return true;
}

/*clear the playerPower */
void clearPlayerPower()
{
	for (unsigned player = 0; player < MAX_PLAYERS; player++)
	{
		asPower[player].currentPower = 0;
		asPower[player].powerModifier = 100;
		asPower[player].powerQueue.clear();
	}
}

/// Returns true iff the power is available. New requests replace old ones (without losing the position in the queue).
static bool addPowerRequest(unsigned player, unsigned id, int64_t amount)
{
	PlayerPower *p = &asPower[player];

	int64_t requiredPower = amount;
	size_t n;
	for (n = 0; n < p->powerQueue.size() && p->powerQueue[n].id != id; ++n)
	{
		requiredPower += p->powerQueue[n].amount;
	}
	if (n == p->powerQueue.size())
	{
		p->powerQueue.resize(n + 1);
		p->powerQueue[n].id = id;
	}
	p->powerQueue[n].amount = amount;
	return requiredPower <= p->currentPower;
}

void delPowerRequest(STRUCTURE *psStruct)
{
	PlayerPower *p = &asPower[psStruct->player];

	for (size_t n = 0; n < p->powerQueue.size(); ++n)
	{
		if (p->powerQueue[n].id == psStruct->id)
		{
			p->powerQueue.erase(p->powerQueue.begin() + n);
			return;
		}
	}
}

static int64_t checkPrecisePowerRequest(STRUCTURE *psStruct)
{
	PlayerPower const *p = &asPower[psStruct->player];

	int64_t requiredPower = 0;
	for (size_t n = 0; n < p->powerQueue.size(); ++n)
	{
		requiredPower += p->powerQueue[n].amount;
		if (p->powerQueue[n].id == psStruct->id)
		{
			if (requiredPower <= p->currentPower)
			{
				return -1;  // Have enough power.
			}
			return requiredPower - p->currentPower;
		}
	}

	return -1;
}

int32_t checkPowerRequest(STRUCTURE *psStruct)
{
	int64_t power = checkPrecisePowerRequest(psStruct);
	return power != -1 ? power / FP_ONE : -1;
}

static int64_t getPreciseQueuedPower(unsigned player)
{
	PlayerPower const *p = &asPower[player];

	int64_t requiredPower = 0;
	for (size_t n = 0; n < p->powerQueue.size(); ++n)
	{
		requiredPower += p->powerQueue[n].amount;
	}
	return requiredPower;
}

int getQueuedPower(int player)
{
	PlayerPower const *p = &asPower[player];

	int64_t requiredPower = 0;
	for (size_t n = 0; n < p->powerQueue.size(); ++n)
	{
		requiredPower += p->powerQueue[n].amount;
	}
	return requiredPower / FP_ONE;
}

static void syncDebugEconomy(unsigned player, char ch)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player (%d)", player);

	syncDebug("%c economy%u = %" PRId64"", ch, player, asPower[player].currentPower);
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

	return asPower[player].currentPower >= quantity * FP_ONE;
}

void usePower(int player, uint32_t quantity)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player (%d)", player);
	syncDebug("usePower%d %" PRId64"-=%u", player, asPower[player].currentPower, quantity);
	asPower[player].currentPower = MAX(0, asPower[player].currentPower - quantity * FP_ONE);
}

void addPower(int player, int32_t quantity)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Bad player (%d)", player);
	syncDebug("addPower%d %" PRId64"+=%d", player, asPower[player].currentPower, quantity);
	asPower[player].currentPower += quantity * FP_ONE;
	CLIP(asPower[player].currentPower, 0, MAX_POWER * FP_ONE);
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
	int64_t                 extractedPoints;

	pResExtractor = (RES_EXTRACTOR *) psBuilding->pFunctionality;
	extractedPoints = 0;

	//only extracts points whilst its active ie associated with a power gen
	//and has got some power to extract
	if (pResExtractor->psPowerGen != nullptr)
	{
		// include modifier as a %
		extractedPoints = asPower[psBuilding->player].powerModifier * EXTRACT_POINTS * FP_ONE / (100 * GAME_UPDATES_PER_SEC);
		syncDebug("updateExtractedPower%d = %" PRId64"", psBuilding->player, extractedPoints);
	}
	ASSERT(extractedPoints >= 0, "extracted negative amount of power");
	return extractedPoints;
}

//returns the relevant list based on OffWorld or OnWorld
STRUCTURE *powerStructList(int player)
{
	ASSERT_OR_RETURN(NULL, player < MAX_PLAYERS, "Invalid player %d", player);
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
void updatePlayerPower(int player, int ticks)
{
	STRUCTURE		*psStruct;//, *psList;
	int64_t powerBefore = asPower[player].currentPower;

	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player %d", player);

	syncDebugEconomy(player, '<');

	for (psStruct = powerStructList(player); psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_POWER_GEN && psStruct->status == SS_BUILT)
		{
			updateCurrentPower(psStruct, player, ticks);
		}
	}
	syncDebug("updatePlayerPower%u %" PRId64"->%" PRId64"", player, powerBefore, asPower[player].currentPower);

	syncDebugEconomy(player, '<');
}

/* Updates the current power based on the extracted power and a Power Generator*/
static void updateCurrentPower(STRUCTURE *psStruct, UDWORD player, int ticks)
{
	POWER_GEN *psPowerGen = (POWER_GEN *)psStruct->pFunctionality;
	int i;
	int64_t extractedPower;

	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player %u", player);

	//each power gen can cope with its associated resource extractors
	extractedPower = 0;
	for (i = 0; i < NUM_POWER_MODULES; i++)
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

	int multiplier = getBuildingPowerPoints(psStruct);
	syncDebug("updateCurrentPower%d = %" PRId64",%u", player, extractedPower, multiplier);

	asPower[player].currentPower += (extractedPower * multiplier) / 100 * ticks;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
	if (asPower[player].currentPower > MAX_POWER * FP_ONE)
	{
		asPower[player].currentPower = MAX_POWER * FP_ONE;
	}
}

void setPower(unsigned player, int32_t power)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player (%u)", player);

	syncDebug("setPower%d %" PRId64"->%d", player, asPower[player].currentPower, power);
	asPower[player].currentPower = power * FP_ONE;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
}

void setPrecisePower(unsigned player, int64_t power)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player (%u)", player);

	syncDebug("setPower%d %" PRId64"->%" PRId64"", player, asPower[player].currentPower, power);
	asPower[player].currentPower = power;
	ASSERT(asPower[player].currentPower >= 0, "negative power");
}

int32_t getPower(unsigned player)
{
	ASSERT_OR_RETURN(0, player < MAX_PLAYERS, "Invalid player (%u)", player);

	return asPower[player].currentPower / FP_ONE;
}

int64_t getPrecisePower(unsigned player)
{
	ASSERT_OR_RETURN(0, player < MAX_PLAYERS, "Invalid player (%u)", player);

	return asPower[player].currentPower;
}

int32_t getPowerMinusQueued(unsigned player)
{
	ASSERT_OR_RETURN(0, player < MAX_PLAYERS, "Invalid player (%u)", player);

	return (asPower[player].currentPower - getPreciseQueuedPower(player)) / FP_ONE;
}

bool requestPowerFor(STRUCTURE *psStruct, int32_t amount)
{
	return requestPrecisePowerFor(psStruct, amount * FP_ONE);
}

bool requestPrecisePowerFor(STRUCTURE *psStruct, int64_t amount)
{
	if (amount <= 0 || !powerCalculated)
	{
		return true;
	}

	bool haveEnoughPower = addPowerRequest(psStruct->player, psStruct->id, amount);
	if (haveEnoughPower)
	{
		// you can have it
		asPower[psStruct->player].currentPower -= amount;
		delPowerRequest(psStruct);
		syncDebug("requestPrecisePowerFor%d,%u amount%" PRId64"", psStruct->player, psStruct->id, amount);
		return true;
	}
	syncDebug("requestPrecisePowerFor%d,%u wait,amount%" PRId64"", psStruct->player, psStruct->id, amount);
	return false;  // Not enough power in the queue.
}
