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
/** \file
 *  Type aliases for per-player object list storage.
 */

#pragma once

#include <array>
#include <list>

#include "lib/framework/frame.h" // MAX_PLAYERS

struct BASE_OBJECT;
struct DROID;
struct STRUCTURE;
struct FEATURE;
struct FLAG_POSITION;

template <typename ObjectType, unsigned PlayerCount>
using PerPlayerObjectLists = std::array<std::list<ObjectType*>, PlayerCount>;

using PerPlayerDroidLists = PerPlayerObjectLists<DROID, MAX_PLAYERS>;
using DroidList = typename PerPlayerDroidLists::value_type;

using PerPlayerStructureLists = PerPlayerObjectLists<STRUCTURE, MAX_PLAYERS>;
using StructureList = typename PerPlayerStructureLists::value_type;

using PerPlayerFeatureLists = PerPlayerObjectLists<FEATURE, 1>;
using FeatureList = typename PerPlayerFeatureLists::value_type;

using PerPlayerFlagPositionLists = PerPlayerObjectLists<FLAG_POSITION, MAX_PLAYERS>;
using FlagPositionList = typename PerPlayerFlagPositionLists::value_type;

using PerPlayerExtractorLists = PerPlayerStructureLists;
using ExtractorList = typename PerPlayerExtractorLists::value_type;

using GlobalSensorList = PerPlayerObjectLists<BASE_OBJECT, 1>;
using SensorList = typename GlobalSensorList::value_type;

using GlobalOilList = PerPlayerObjectLists<FEATURE, 1>;
using OilList = typename GlobalOilList::value_type;

using DestroyedObjectsList = std::list<BASE_OBJECT*>;
