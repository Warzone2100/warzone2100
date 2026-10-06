// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "hudobjectinfo.h"
#include "hudtext.h"

#include "lib/framework/frame.h"

#include "../droid.h"
#include "../intdisplay.h"
#include "../power.h"
#include "../research.h"
#include "../structure.h"

unsigned HudJob::percentDone() const
{
	return totalPoints > 0 ? std::min(donePoints, totalPoints) * 100 / totalPoints : 0;
}

unsigned HudJob::secondsLeft() const
{
	return hudWorkSeconds(totalPoints > donePoints ? totalPoints - donePoints : 0, pointsPerSecond);
}

nonstd::optional<HudJob> hudStructureJob(STRUCTURE *psStruct)
{
	if (psStruct == nullptr || psStruct->died || psStruct->status != SS_BUILT)
	{
		return nonstd::nullopt;
	}
	HudJob job;
	if (psStruct->isFactory())
	{
		FACTORY *factory = StructureGetFactory(psStruct);
		DROID_TEMPLATE *psTemplate = FactoryGetTemplate(factory);
		job.pointsPerSecond = std::max(0, getBuildingProductionPoints(psStruct));
		job.loops = factory->productionLoops;
		if (psTemplate != nullptr)
		{
			job.subject = psTemplate;
			job.totalPoints = calcTemplateBuild(psTemplate);
			if (factory->psSubject == psTemplate)
			{
				const int remaining = std::max(0, std::min<int>(factory->buildPointsRemaining, job.totalPoints));
				job.donePoints = job.totalPoints - static_cast<unsigned>(remaining);
			}
		}
		for (const auto &entry : hudFactoryQueue(psStruct))
		{
			job.queued += entry.remaining;
			if (psTemplate != nullptr && entry.psTemplate->multiPlayerID == psTemplate->multiPlayerID && job.queuedOfSubject == 0)
			{
				job.queuedOfSubject = entry.remaining;
			}
		}
	}
	else if (psStruct->pStructureType->type == REF_RESEARCH)
	{
		RESEARCH_FACILITY *facility = StructureGetResearch(psStruct);
		RESEARCH *topic = facility->psSubjectPending != nullptr ? facility->psSubjectPending : facility->psSubject;
		job.pointsPerSecond = std::max(0, getBuildingResearchPoints(psStruct));
		if (topic != nullptr && psStruct->player < MAX_PLAYERS && topic->index < asPlayerResList[psStruct->player].size())
		{
			job.subject = topic;
			job.totalPoints = topic->researchPoints;
			job.donePoints = std::min<unsigned>(job.totalPoints, asPlayerResList[psStruct->player][topic->index].currentPoints);
		}
	}
	else
	{
		return nonstd::nullopt;
	}
	job.onHold = StructureIsOnHoldPending(psStruct);
	if (job.subject != nullptr)
	{
		const int32_t needed = checkPowerRequest(psStruct);
		if (needed >= 0)
		{
			job.powerNeeded = needed;
		}
	}
	return job;
}

std::vector<HudQueueEntry> hudFactoryQueue(STRUCTURE *psStruct)
{
	std::vector<HudQueueEntry> queue;
	if (psStruct == nullptr || !psStruct->isFactory())
	{
		return queue;
	}
	FACTORY *factory = StructureGetFactory(psStruct);
	if (factory->psAssemblyPoint == nullptr)
	{
		return queue;
	}
	const auto &runs = asProductionRun[factory->psAssemblyPoint->factoryType];
	if (factory->psAssemblyPoint->factoryInc >= runs.size())
	{
		return queue;
	}
	DROID_TEMPLATE *current = FactoryGetTemplate(factory);
	for (const auto &entry : runs[factory->psAssemblyPoint->factoryInc])
	{
		if (entry.psTemplate == nullptr || entry.numRemaining() <= 0)
		{
			continue;
		}
		HudQueueEntry queued;
		queued.psTemplate = entry.psTemplate;
		queued.remaining = static_cast<unsigned>(entry.numRemaining());
		if (current != nullptr && entry == current)
		{
			queue.insert(queue.begin(), queued);
		}
		else
		{
			queue.push_back(queued);
		}
	}
	return queue;
}
