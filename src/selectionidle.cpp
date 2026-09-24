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

/** \file
 *  Idle factories, research facilities and trucks: finding and selecting them
 */

#include "lib/framework/frame.h"
#include "selectionidle.h"

#include "console.h"
#include "display3d.h"
#include "droid.h"
#include "intdisplay.h"
#include "map.h"
#include "multiplay.h"
#include "objmem.h"
#include "qtscript.h"
#include "warcam.h"
#include "world_object_state.h"

bool structureIsIdlePending(STRUCTURE *structure)
{
	ASSERT_NOT_NULLPTR_OR_RETURN(false, structure);
	if (structure->status != SS_BUILT || structure->died)
	{
		return false;
	}
	switch (structure->pStructureType->type)
	{
	case REF_FACTORY:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
		return !StructureIsManufacturingPending(structure);
	case REF_RESEARCH:
		return !structureIsResearchingPending(structure);
	default:
		return false;
	}
}

bool droidIsIdleTruck(const DROID *psDroid)
{
	ASSERT_NOT_NULLPTR_OR_RETURN(false, psDroid);
	if (!psDroid->isConstructionDroid() || psDroid->died)
	{
		return false;
	}
	if (psDroid->asOrderList.size() > psDroid->listPendingBegin)
	{
		return false;
	}
	const bool noOrder = psDroid->order.type == DORDER_NONE || psDroid->order.type == DORDER_HOLD
		|| (psDroid->order.type == DORDER_GUARD && psDroid->order.psObj == nullptr);
	return noOrder && psDroid->action == DACTION_NONE;
}

static bool isFactoryType(STRUCTURE_TYPE type)
{
	return type == REF_FACTORY || type == REF_CYBORG_FACTORY || type == REF_VTOL_FACTORY;
}

bool selNextIdleBuilding(WorldObjectState& objState, STRUCTURE_TYPE structType, bool jump)
{
	ASSERT_OR_RETURN(false, selectedPlayer < MAX_PLAYERS, "invalid selectedPlayer: %" PRIu32 "", selectedPlayer);
	auto matches = [structType](const STRUCTURE *psStruct) {
		const STRUCTURE_TYPE type = psStruct->pStructureType->type;
		return isFactoryType(structType) ? isFactoryType(type) : type == structType;
	};

	STRUCTURE *psFirst = nullptr, *psResult = nullptr;
	bool afterSelected = false;
	for (STRUCTURE *psCurr : objState.structures[selectedPlayer])
	{
		if (!matches(psCurr))
		{
			continue;
		}
		if (psCurr->selected)
		{
			afterSelected = true;
			continue;
		}
		if (!structureIsIdlePending(psCurr))
		{
			continue;
		}
		if (psFirst == nullptr)
		{
			psFirst = psCurr;
		}
		if (afterSelected)
		{
			psResult = psCurr;
			break;
		}
	}
	if (psResult == nullptr)
	{
		psResult = psFirst;
	}
	if (psResult == nullptr)
	{
		for (STRUCTURE *psCurr : objState.structures[selectedPlayer])
		{
			if (matches(psCurr) && psCurr->selected && structureIsIdlePending(psCurr))
			{
				psResult = psCurr;
				break;
			}
		}
	}
	if (psResult == nullptr)
	{
		addConsoleMessage(isFactoryType(structType) ? _("No idle factories") : _("No idle research facilities"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		return false;
	}

	if (getWarCamStatus())
	{
		camToggleStatus();
	}
	if (jump)
	{
		setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), false);
	}
	for (STRUCTURE *psCurr : objState.structures[selectedPlayer])
	{
		if (matches(psCurr))
		{
			psCurr->selected = (psCurr == psResult);
		}
	}
	triggerEventSelected();
	jsDebugSelected(psResult);
	return true;
}

static uint32_t lastIdleTruckId = 0;

bool selNextIdleTruck(WorldObjectState& objState, bool jump)
{
	ASSERT_OR_RETURN(false, selectedPlayer < MAX_PLAYERS, "invalid selectedPlayer: %" PRIu32 "", selectedPlayer);

	DROID *psFirst = nullptr, *psResult = nullptr;
	bool afterLast = false;
	for (DROID *psCurr : objState.droids[selectedPlayer])
	{
		if (psCurr->id == lastIdleTruckId)
		{
			afterLast = true;
			continue;
		}
		if (!droidIsIdleTruck(psCurr) || !isSelectable(psCurr))
		{
			continue;
		}
		if (psFirst == nullptr)
		{
			psFirst = psCurr;
		}
		if (afterLast)
		{
			psResult = psCurr;
			break;
		}
	}
	if (psResult == nullptr)
	{
		psResult = psFirst;
	}
	if (psResult == nullptr)
	{
		DROID *psLast = IdToDroid(objState, lastIdleTruckId, selectedPlayer);
		if (psLast != nullptr && droidIsIdleTruck(psLast) && isSelectable(psLast))
		{
			psResult = psLast;
		}
	}
	if (psResult == nullptr)
	{
		addConsoleMessage(_("No idle trucks"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		return false;
	}

	lastIdleTruckId = psResult->id;
	selSetDroidSelection(objState, selectedPlayer, [psResult](const DROID *psDroid) { return psDroid == psResult; });
	if (jump)
	{
		if (getWarCamStatus())
		{
			camToggleStatus();
		}
		setViewPos(map_coord(psResult->pos.x), map_coord(psResult->pos.y), true);
	}
	return true;
}

unsigned selAllIdleTrucks(WorldObjectState& objState)
{
	ASSERT_OR_RETURN(0, selectedPlayer < MAX_PLAYERS, "invalid selectedPlayer: %" PRIu32 "", selectedPlayer);
	unsigned count = selSetDroidSelection(objState, selectedPlayer, [](const DROID *psDroid) { return droidIsIdleTruck(psDroid); });
	if (count == 0)
	{
		addConsoleMessage(_("No idle trucks"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
	}
	return count;
}
