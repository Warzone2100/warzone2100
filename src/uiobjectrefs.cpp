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
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "uiobjectrefs.h"

#include "lib/framework/frame.h"

#include "console.h"
#include "display.h"
#include "droid.h"
#include "effects.h"
#include "feature.h"
#include "game_world.h"
#include "hci.h"
#include "keybind.h"
#include "multiplay.h"
#include "screens/spectatorgameoverscreen.h"
#include "structure.h"
#include "terrain.h"

#include <unordered_set>

static bool uiObjectRefsEnabled = true;

void visitUiObjectRefs(const UiObjectRefVisitor &visit)
{
	hciVisitUiObjectRefs(visit);
	intOrderVisitUiObjectRefs(visit);
	transporterVisitUiObjectRefs(visit);
	warCamVisitUiObjectRefs(visit);
	display3dVisitUiObjectRefs(visit);
	displayVisitUiObjectRefs(visit);
	selectionVisitUiObjectRefs(visit);
}

void uiObjectFreed(const BASE_OBJECT *psObj)
{
	if (!uiObjectRefsEnabled)
	{
		return;
	}
	visitUiObjectRefs([psObj](const BASE_OBJECT *ref) { return ref == psObj; });
}

void setUiObjectRefsEnabled(bool enabled)
{
	uiObjectRefsEnabled = enabled;
}

size_t checkUiObjectRefs()
{
	// Every object not yet freed is in its global container, wherever it is (a world's lists, a transporter, limbo,
	// the destroyed list)
	std::unordered_set<const BASE_OBJECT *> live;
	for (const DROID &droid : GlobalDroidContainer())
	{
		live.insert(&droid);
	}
	for (const STRUCTURE &structure : GlobalStructContainer())
	{
		live.insert(&structure);
	}
	for (const FEATURE &feature : GlobalFeatureContainer())
	{
		live.insert(&feature);
	}
	size_t stale = 0;
	visitUiObjectRefs([&](const BASE_OBJECT *ref) {
		if (live.count(ref) == 0)
		{
			++stale;
			ASSERT(false, "The UI holds a reference to a freed object (%p)", static_cast<const void *>(ref));
		}
		return false;
	});
	return stale;
}

UiViewerState uiBeforeObjectsReplaced()
{
	UiViewerState state;
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (const DROID *psDroid : gameWorld.objects.droids[player])
		{
			if (psDroid->selected)
			{
				state.selectedDroids.emplace_back(psDroid->id, player);
			}
		}
		for (const STRUCTURE *psStruct : gameWorld.objects.structures[player])
		{
			if (psStruct->selected)
			{
				state.selectedStructures.emplace_back(psStruct->id, player);
			}
		}
	}
	intResetScreen(true);  // the forms, the intel and transporter screens, build placement
	closeSpectatorGameOverScreen();  // a replay's end, shown until a seek goes back from it
	clearActiveConsole();            // the messages on screen (the history keeps them)
	cancelDeliveryRepos();
	initEffectsSystem();   // explosions, smoke and the like of the world being left
	keybindShutdown();     // the extractor cycling's place in the extractor list
	return state;
}

void uiAfterObjectsReplaced(const UiViewerState &state)
{
	for (const auto &droid : state.selectedDroids)
	{
		if (DROID *psDroid = IdToDroid(gameWorld.objects, droid.first, droid.second))
		{
			psDroid->selected = true;
		}
	}
	for (const auto &structure : state.selectedStructures)
	{
		if (STRUCTURE *psStruct = IdToStruct(structure.first, structure.second))
		{
			psStruct->selected = true;
		}
	}
	intRefreshScreen();
	dirtyAllSectors();  // the restore wrote the tiles' heights and textures directly
}
