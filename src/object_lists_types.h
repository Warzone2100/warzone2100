// SPDX-License-Identifier: GPL-2.0-or-later

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
