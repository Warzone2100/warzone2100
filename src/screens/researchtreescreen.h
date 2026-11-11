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
/** @file
 *  Full screen host for the research tree canvas.
 */

#ifndef __INCLUDED_SRC_SCREENS_RESEARCHTREESCREEN_H__
#define __INCLUDED_SRC_SCREENS_RESEARCHTREESCREEN_H__

#include "../researchtree/researchtreelayout.h"

#include <functional>
#include <memory>

struct W_SCREEN;

// Open the research tree over the game, or close it if it is already open.
// Does nothing without a valid player slot. Returns whether it is now open.
bool toggleResearchTreeScreen();

// Open the research tree over whatever is already on show. A screen sitting on
// its own overlay, ex. the spectator game over screen, has to name itself or the
// tree opens underneath it.
void showResearchTreeScreen(const ResearchTreeContext& context, const std::shared_ptr<W_SCREEN>& aboveScreen = nullptr);
void closeResearchTreeScreen();
bool researchTreeScreenIsUp();

// Driven by the research tree key mappings, which are only live while the view
// is open. Each does nothing when it is not.
void researchTreeScreenBack();			// give up one thing, closing once there is nothing left
void researchTreeScreenTracePath();
void researchTreeScreenFocusSearch();
void researchTreeScreenToggleNames();
void researchTreeScreenStepSelection(int delta);
void researchTreeScreenCyclePerspective(int delta);	// whose research is on show

#endif // __INCLUDED_SRC_SCREENS_RESEARCHTREESCREEN_H__
