/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "researchassign.h"

#include "lib/gamelib/gtime.h"

#include "../hci.h"
#include "../power.h"
#include "../research.h"
#include "../statsdef.h"
#include "../structure.h"

#include <algorithm>

std::vector<ResearchLabOption> researchLabsFor(uint32_t player)
{
	std::vector<ResearchLabOption> options;
	if (player >= MAX_PLAYERS || player != selectedPlayer)
	{
		return options;		// only the player at the keyboard has labs to offer
	}
	const StructureList *structures = interfaceStructList();
	if (structures == nullptr)
	{
		return options;
	}

	for (STRUCTURE *psStruct : *structures)
	{
		if (psStruct->pStructureType->type != REF_RESEARCH || psStruct->status != SS_BUILT || psStruct->died != 0)
		{
			continue;		// still going up, or gone
		}
		const RESEARCH_FACILITY *psFacility = &psStruct->pFunctionality->researchFacility;

		ResearchLabOption option;
		option.facilityId = psStruct->id;
		option.changing = (psFacility->statusPending != FACTORY_NOTHING_PENDING);
		option.pointsPerSecond = getBuildingResearchPoints(psStruct);
		option.modules = psStruct->capacity;
		option.idle = (psFacility->psSubject == nullptr && psFacility->psSubjectPending == nullptr);
		option.onHold = (psFacility->psSubject != nullptr && psFacility->timeStartHold != 0);
		option.waitingForPower = (checkPowerRequest(psStruct) != -1);

		const RESEARCH *subject = psFacility->psSubject ? psFacility->psSubject : psFacility->psSubjectPending;
		if (subject != nullptr)
		{
			option.subjectIndex = static_cast<uint16_t>(subject->ref - STAT_RESEARCH);
			option.currentSubject = WzString::fromUtf8(getLocalizedStatsName(subject));
			const size_t index = static_cast<size_t>(subject->ref - STAT_RESEARCH);
			const auto& playerList = asPlayerResList[player];
			if (index < playerList.size() && subject->researchPoints > 0)
			{
				option.currentPercent = static_cast<int>(
					static_cast<uint64_t>(playerList[index].currentPoints) * 100 / subject->researchPoints);
			}
		}
		options.push_back(std::move(option));
	}

	// A fixed order, so a lab keeps its place in a row of them
	std::sort(options.begin(), options.end(), [](const ResearchLabOption& a, const ResearchLabOption& b) {
		return a.facilityId < b.facilityId;
	});
	return options;
}

std::vector<ResearchLabOption> researchLabOptionsFor(uint32_t player, const RESEARCH& research)
{
	std::vector<ResearchLabOption> options = researchLabsFor(player);
	// A lab with a message still in flight is not worth offering, since what it is
	// about to be doing is not settled.
	//
	// But that holds only while the clock is running (which is what settles it):
	// an order is handed back as game time advances, so with the clock stopped
	// it never is, and waiting for it would mean waiting for whatever stopped
	// the clock to resume it. In this case, offer the lab anyway. Orders stack,
	// arrive in the order they were given, and the last order wins.
	if (!gameTimeIsStopped())
	{
		options.erase(std::remove_if(options.begin(), options.end(),
		                             [](const ResearchLabOption& option) { return option.changing; }),
		              options.end());
	}
	for (auto& option : options)
	{
		option.researchingThis = option.currentSubject.isEmpty() ? false : (option.subjectIndex == research.index);
	}

	// Idle first, since those displace nothing, then whichever finishes soonest.
	// Ties go to the lower structure id so the same press always does the same thing.
	std::stable_sort(options.begin(), options.end(), [](const ResearchLabOption& a, const ResearchLabOption& b) {
		if (a.idle != b.idle) { return a.idle; }
		return a.pointsPerSecond > b.pointsPerSecond;
	});
	return options;
}

bool canStartResearchNow(uint32_t player, uint16_t researchIndex)
{
	if (player >= MAX_PLAYERS || player != selectedPlayer || researchIndex >= asResearch.size())
	{
		return false;
	}
	return researchAvailable(researchIndex, player, ModeQueue);
}
