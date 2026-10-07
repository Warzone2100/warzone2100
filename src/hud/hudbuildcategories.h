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
/** @file
 *  Build categories, for the build panel's filters and the structures' detail popovers.
 *  Defined in stats/buildcategories.json (mods and campaigns may ship their own). A structure names its own in
 *  structure.json ("buildCategory"), or gets one derived from its type, so mods need no changes.
 */

#ifndef __INCLUDED_SRC_HUD_HUDBUILDCATEGORIES_H__
#define __INCLUDED_SRC_HUD_HUDBUILDCATEGORIES_H__

#include "lib/framework/wzstring.h"

#include <string>
#include <vector>

struct STRUCTURE_STATS;

struct HudBuildCategory
{
	std::string id;
	WzString name;      ///< translated
	std::string icon;   ///< drawn instead of the name where the skin draws icons (images/hud/icons/<icon>.svg), or empty
	int order = 0;
};

/// The categories, in their order (loaded once per game, "other" always last)
const std::vector<HudBuildCategory> &hudBuildCategories();
/// The category a structure is in: its own, or derived from its type
std::string hudBuildCategoryOf(const STRUCTURE_STATS *stats);
const HudBuildCategory *hudFindBuildCategory(const std::string &id);
/// What the type of a structure alone says its category is (the fallback)
std::string hudDerivedBuildCategory(const STRUCTURE_STATS *stats);

/// (src/structure.cpp) A structure's own "buildCategory", as its stats are loaded
void hudSetStructureBuildCategory(const std::string &structureId, const std::string &category);
/// Forget what was loaded (stats are loaded again)
void hudResetBuildCategories();
/// Log every structure's category, and where it comes from (for mod authors: the debug command "buildcategories")
void hudLogBuildCategories();

#endif // __INCLUDED_SRC_HUD_HUDBUILDCATEGORIES_H__
