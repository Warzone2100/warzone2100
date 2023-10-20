/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "../statsdef.h"

#include <unordered_map>

struct NETQUEUE;

enum class WzStrategyPlanningState : uint8_t
{
	NotSpecified,
	Planned,
	HeavilyPlanned,
	Never,
};

enum class WzStrategyPlanningUnitTypes : uint8_t
{
	Tanks,
	VTOL,
	Cyborg
};
constexpr size_t WzStrategyPlanningUnitTypes_NUM_TYPES = 3;

bool gameHasTeamStrategyView(bool allowAIs = false);
std::shared_ptr<WIDGET> createTeamStrategyView(bool allowAIs = false);
bool transformTeamStrategyViewMode(const std::shared_ptr<WIDGET>& teamStrategyView, bool updatingPlayers);
bool teamStrategyViewSetBackgroundColor(const std::shared_ptr<WIDGET>& view, PIELIGHT color);

bool recvStrategyPlanUpdate(NETQUEUE queue);

// Called to inform all human players on an AI's team of the AI's (current) strategy
void aiInformTeamOfStrategy(uint32_t aiPlayerIdx, const std::unordered_map<WEAPON_SUBCLASS, WzStrategyPlanningState>& weaponStrategy, const std::unordered_map<WzStrategyPlanningUnitTypes, WzStrategyPlanningState>& unitTypesStrategy);

int32_t checkedGetPlayerTeam(int32_t i);

void strategyPlansInit();
