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

#include "hudmodel.h"
#include "hudobjectinfo.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"

#include "../droid.h"
#include "../game_world.h"
#include "../intdisplay.h"
#include "../selectionidle.h"
#include "../loop.h"
#include "../objmem.h"
#include "../power.h"
#include "../research.h"
#include "../structure.h"

constexpr int64_t POWER_FIXED_POINT_ONE = int64_t(1) << 32;
constexpr uint32_t MAX_GAME_TIME_STEP_MS = 15 * 1000;
constexpr uint32_t MIN_INCOME_WINDOW_MS = 1000;

HudModel &hudModel()
{
	static HudModel model;
	return model;
}

void HudModel::reset()
{
	powerInfo = Power();
	unitsInfo = Units();
	researchInfo = Research();
	idleInfo = Idle();
	powerSamples.clear();
	lastUpdateGameTime = nonstd::nullopt;
}

void HudModel::update()
{
	if (lastUpdateGameTime.has_value())
	{
		const uint32_t last = lastUpdateGameTime.value();
		if (gameTime < last || gameTime - last > MAX_GAME_TIME_STEP_MS)
		{
			reset();
		}
	}
	lastUpdateGameTime = gameTime;
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	updatePower(selectedPlayer);
	updateObjects(selectedPlayer);
}

std::string HudModel::Power::derrickSummary() const
{
	return astringf("%s · %s · %s", astringf(ngettext("%u derrick", "%u derricks", derricks), derricks).c_str(),
	                astringf(ngettext("%u generator", "%u generators", generators), generators).c_str(),
	                astringf(_("%u unpowered"), unusedDerricks).c_str());
}

void HudModel::updateObjects(unsigned player)
{
	unitsInfo.count = getNumDroids(player) + getNumMissionDroids(player) + getNumTransporterDroids(player);
	unitsInfo.limit = static_cast<unsigned>(std::max(0, getMaxDroids(player)));
	unitsInfo.atLimit = IsPlayerDroidLimitReached(player);

	researchInfo = Research();
	idleInfo = Idle();
	for (STRUCTURE *psStruct : gameWorld.objects.structures[player])
	{
		if (psStruct->status != SS_BUILT || psStruct->died)
		{
			continue;
		}
		const STRUCTURE_TYPE type = psStruct->pStructureType->type;
		if (type == REF_RESEARCH)
		{
			++researchInfo.facilities;
			const auto job = hudStructureJob(psStruct);
			if (job.has_value() && job->subject != nullptr)
			{
				++researchInfo.active;
				researchInfo.onHold += job->onHold ? 1 : 0;
				researchInfo.waitingForPower += job->powerNeeded.has_value() && !job->onHold ? 1 : 0;
				if (!job->onHold && job->pointsPerSecond > 0
					&& (!researchInfo.nextTopic.has_value() || job->secondsLeft() < researchInfo.nextSeconds))
				{
					researchInfo.nextTopic = static_cast<uint16_t>(static_cast<RESEARCH *>(job->subject)->index);
					researchInfo.nextSeconds = job->secondsLeft();
				}
			}
		}
		if (structureIsIdlePending(psStruct))
		{
			(type == REF_RESEARCH ? idleInfo.researchFacilities : idleInfo.factories) += 1;
		}
	}
	for (const DROID *psDroid : gameWorld.objects.droids[player])
	{
		idleInfo.trucks += droidIsIdleTruck(psDroid) ? 1 : 0;
	}
}

void HudModel::updatePower(unsigned player)
{
	powerInfo.current = getPower(player);
	powerInfo.afterQueued = getPowerMinusQueued(player);
	powerInfo.queued = getQueuedPower(player);
	powerInfo.maxStorage = static_cast<int>(getPlayerPowerState(player).maxStorage / POWER_FIXED_POINT_ONE);
	powerInfo.unusedDerricks = countPlayerUnusedDerricks(gameWorld.objects);

	unsigned derricks = 0;
	for (const STRUCTURE *psStruct : gameWorld.objects.extractors[player])
	{
		if (psStruct->status == SS_BUILT && psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			++derricks;
		}
	}
	unsigned generators = 0;
	for (const STRUCTURE *psStruct : gameWorld.objects.structures[player])
	{
		if (psStruct->status == SS_BUILT && psStruct->pStructureType->type == REF_POWER_GEN)
		{
			++generators;
		}
	}
	powerInfo.derricks = derricks;
	powerInfo.generators = generators;

	const int64_t extracted = getExtractedPower(player);
	if (powerSamples.empty() || powerSamples.back().gameTime != gameTime)
	{
		powerSamples.push_back({gameTime, extracted});
	}
	while (powerSamples.size() > 2 && gameTime - powerSamples[1].gameTime >= INCOME_WINDOW_MS)
	{
		powerSamples.pop_front();
	}
	const PowerSample &first = powerSamples.front();
	const PowerSample &last = powerSamples.back();
	const uint32_t window = last.gameTime - first.gameTime;
	powerInfo.incomeWindowMs = window;
	if (window >= MIN_INCOME_WINDOW_MS)
	{
		const double perMs = static_cast<double>(last.extracted - first.extracted) / window;
		powerInfo.incomePerMinute = static_cast<int>(perMs * 60000.0 + 0.5);
	}
	else
	{
		powerInfo.incomePerMinute = nonstd::nullopt;
	}
}
