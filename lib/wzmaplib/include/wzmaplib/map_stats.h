/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

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

#include "map_types.h"
#include "map_debug.h"
#include "map_io.h"

#include <string>
#include <unordered_set>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

namespace WzMap {

	struct MapStats
	{
		struct StartEquality
		{
			// Whether all players have equal starting units (quantity and type)
			bool units = true;
			// Whether all players have equal starting structures (of all types - buildings, walls, etc)
			bool structures = true;
			// Whether all players have equal starting resource extractors (i.e. oil derricks)
			bool resourceExtractors = true;
			// Whether all players have equal starting power generators
			bool powerGenerators = true;
			// Whether all players have equal starting factories (of all types)
			bool factories = true;
			// Whether all players have equal starting factories (of each type)
			bool regFactories = true;
			bool vtolFactories = true;
			bool cyborgFactories = true;
			// Whether all players have equal starting research centers
			bool researchCenters = true;
		};

		struct PerPlayerCounts
		{
			struct MinMax
			{
				uint32_t min = 0;
				uint32_t max = 0;
			};
			MinMax unitsPerPlayer;
			MinMax structuresPerPlayer;
			MinMax resourceExtractorsPerPlayer;
			MinMax powerGeneratorsPerPlayer;
			MinMax powerGeneratorModulesPerPlayer;
			MinMax regFactoriesPerPlayer;
			MinMax regFactoryModulesPerPlayer;
			MinMax vtolFactoriesPerPlayer;
			MinMax cyborgFactoriesPerPlayer;
			MinMax researchCentersPerPlayer;
			MinMax researchCenterModulesPerPlayer;
		};

	public:
		// Basic map properties
		uint32_t mapWidth = 0;
		uint32_t mapHeight = 0;
		// Scavenger counts
		uint32_t scavengerUnits = 0;
		uint32_t scavengerStructs = 0;
		uint32_t scavengerFactories = 0;
		uint32_t scavengerResourceExtractors = 0;
		// The total number of oil resources (oil well features) on the map
		uint32_t oilWellsTotal = 0;
		// Per-player counts of various starting droids and structs
		// NOTE: In certain cases (like with unitsPerPlayer) min == max does not necessarily imply playerBalance.units will be true (as the *type* of units may differ)
		PerPlayerCounts perPlayerCounts;
		// An analysis of the starting balance for all players
		StartEquality playerBalance;
	public:
		inline bool hasScavengers() const { return scavengerUnits > 0 || scavengerStructs > 0; }
	};

	class MapStatsConfiguration
	{
	public:
		// Construct a MapStatsConfiguration with hard-coded defaults that map to WZ 4.3+ (currently)
		MapStatsConfiguration();

		// Load stats configuration for droid templates from a `templates.json` file
		// Returns: `true` if the `templates.json` file was loaded, and overwrote the existing droid stats configuration
		bool loadFromTemplatesJSON(const std::string& templatesJSONPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);

		// Load stats configuration for structures from a `structure.json` file
		// Returns: `true` if the `structure.json` file was loaded, and overwrote the existing structures stats configuration
		bool loadFromStructureJSON(const std::string& structureJSONPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);

		// Load stats configuration for features from a `features.json` file
		// Returns: `true` if the `features.json` file was loaded, and overwrote the existing feature stats configuration
		bool loadFromFeaturesJSON(const std::string& featuresJSONPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);
	public:
		// [DROID TEMPLATES]:
		// the names of any droid templates that are constructor droids
		//	- "type": "CONSTRUCT"
		std::unordered_set<std::string> constructorDroids;

		// [STRUCTS]:
		// the names (ids) of resource extractor structs
		//	- "type": "RESOURCE EXTRACTOR"
		std::unordered_set<std::string> resourceExtractors;
		// the names (ids) of power generator structs
		//	- "type": "POWER GENERATOR"
		std::unordered_set<std::string> powerGenerators;
		// the names (ids) of factory structs
		//	- "type": "FACTORY"
		std::unordered_set<std::string> factories;
		//	- "type": "VTOL FACTORY"
		std::unordered_set<std::string> vtolFactories;
		//	- "type": "CYBORG FACTORY"
		std::unordered_set<std::string> cyborgFactories;
		// the names (ids) of research structs
		//	- "type": "RESEARCH"
		std::unordered_set<std::string> researchCenters;

		// [STRUCT MODULES]:
		//	- "type": "FACTORY MODULE"
		std::unordered_set<std::string> factoryModules;
		//	- "type": "RESEARCH MODULE"
		std::unordered_set<std::string> researchModules;
		//	- "type": "POWER MODULE"
		std::unordered_set<std::string> powerModules;

		// [FEATURES]:
		//	- "type": "OIL RESOURCE"
		std::unordered_set<std::string> oilResources;
		//	- "type": "OIL DRUM"
		std::unordered_set<std::string> oilDrums;
	};

} // namespace WzMap
