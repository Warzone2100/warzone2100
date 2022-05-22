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

#include "../include/wzmaplib/map_stats.h"
#include "../include/wzmaplib/map.h"
#include "map_internal.h"

#include <vector>
#include <unordered_map>

namespace WzMap {

// MARK: - Calculating map stats / info

static void incrementPlayerEntityCount(std::vector<std::unordered_map<std::string, uint32_t>>& playerEntityCounts, size_t player, const std::string& entity)
{
	if (player >= playerEntityCounts.size())
	{
		playerEntityCounts.resize(player + 1);
	}
	playerEntityCounts[player][entity]++;
}

optional<MapStats> Map::calculateMapStats(uint32_t mapMaxPlayers, MapStatsConfiguration statsConfig)
{
	MapStats results;

	auto pMapData = mapData();
	if (!pMapData) { return nullopt; }
	auto pDroids = mapDroids();
	if (!pDroids) { return nullopt; }
	auto pFeatures = mapFeatures();
	if (!pFeatures) { return nullopt; }
	auto pStructures = mapStructures();
	if (!pStructures) { return nullopt; }

	results.mapWidth = pMapData->width;
	results.mapHeight = pMapData->height;

	// Check droids
	std::vector<std::unordered_map<std::string, uint32_t>> playerUnitCounts(mapMaxPlayers);
	for (auto& droid : *pDroids)
	{
		if (droid.player == PLAYER_SCAVENGERS)
		{
			results.scavengerUnits++;
		}
		else if (droid.player < 0)
		{
			// Invalid player
			continue;
		}
		else
		{
			incrementPlayerEntityCount(playerUnitCounts, droid.player, droid.name);
		}
	}
	if (!playerUnitCounts.empty())
	{
		results.playerBalance.units = std::equal(++playerUnitCounts.begin(), playerUnitCounts.end(), playerUnitCounts.begin());
	}
	else
	{
		results.playerBalance.units = true;
	}

	std::string factoryModuleName = (!statsConfig.factoryModules.empty()) ? *(statsConfig.factoryModules.begin()) : "<unknown factory module>";
	std::string researchModuleName = (!statsConfig.researchModules.empty()) ? *(statsConfig.researchModules.begin()) : "<unknown research module>";
	std::string powerModuleName = (!statsConfig.powerModules.empty()) ? *(statsConfig.powerModules.begin()) : "<unknown power module>";

	// Check structures
	std::vector<std::unordered_map<std::string, uint32_t>> playerStructCounts(mapMaxPlayers);
	for (auto& structure : *pStructures)
	{
		if (structure.player == PLAYER_SCAVENGERS)
		{
			results.scavengerStructs++;
			if (statsConfig.resourceExtractors.count(structure.name) > 0)
			{
				results.scavengerResourceExtractors++;
			}
			else if ((statsConfig.factories.count(structure.name) > 0) || (statsConfig.vtolFactories.count(structure.name) > 0) || (statsConfig.cyborgFactories.count(structure.name) > 0))
			{
				results.scavengerFactories++;
			}
		}
		else if (structure.player < 0)
		{
			// Invalid player
			continue;
		}
		else
		{
			incrementPlayerEntityCount(playerStructCounts, structure.player, structure.name);
			if (structure.modules > 0)
			{
				if (statsConfig.powerGenerators.count(structure.name) > 0)
				{
					// simulate addition of a power generator module
					incrementPlayerEntityCount(playerStructCounts, structure.player, powerModuleName);
				}
				else if (statsConfig.researchCenters.count(structure.name) > 0)
				{
					// simulate addition of a research module
					incrementPlayerEntityCount(playerStructCounts, structure.player, researchModuleName);
				}
				else if (statsConfig.factories.count(structure.name) > 0)
				{
					// simulate addition of N factory modules
					for (uint8_t i = 0; i < structure.modules; i++)
					{
						incrementPlayerEntityCount(playerStructCounts, structure.player, factoryModuleName);
					}
				}
			}
		}
	}
	if (!playerStructCounts.empty())
	{
		results.playerBalance.structures = std::equal(++playerStructCounts.begin(), playerStructCounts.end(), playerStructCounts.begin());
	}
	else
	{
		results.playerBalance.structures = true;
	}

	// Check individual types of structures
	if (!playerStructCounts.empty())
	{
		struct StructTypeMapping
		{
			bool* pBalance;
			uint32_t* pPerPlayerCount;
		};
		std::vector<std::pair<std::unordered_set<std::string>*, StructTypeMapping>> structureTypeChecks = {
			// resource extractors (oil derricks)
			{&statsConfig.resourceExtractors, {&results.playerBalance.resourceExtractors, &results.resourceExtractorsPerPlayer}},
			// power generators
			{&statsConfig.powerGenerators, {&results.playerBalance.powerGenerators, &results.powerGeneratorsPerPlayer}},
			{&statsConfig.powerModules, {&results.playerBalance.powerGenerators, nullptr}},
			// factories
			{&statsConfig.factories, {&results.playerBalance.factories, &results.regFactoriesPerPlayer}},
			{&statsConfig.factoryModules, {&results.playerBalance.factories, nullptr}},
			// vtol factories
			{&statsConfig.vtolFactories, {&results.playerBalance.factories, &results.vtolFactoriesPerPlayer}},
			// cyborg factories
			{&statsConfig.cyborgFactories, {&results.playerBalance.factories, &results.cyborgFactoriesPerPlayer}},
			// research centers
			{&statsConfig.researchCenters, {&results.playerBalance.researchCenters, &results.researchCentersPerPlayer}},
			{&statsConfig.researchModules, {&results.playerBalance.researchCenters, nullptr}}
		};
		for (const auto& structureTypeCheck : structureTypeChecks)
		{
			const StructTypeMapping& resultMapping = structureTypeCheck.second;
			if (resultMapping.pPerPlayerCount)
			{
				// For the purposes of getting a count of the entity type, determine the count across all entityNames for each player
				optional<uint32_t> minStructTypeCount;
				for (const auto& structCounts : playerStructCounts)
				{
					uint32_t playerEntityTypeCount = 0;
					for (const auto& entityName : *structureTypeCheck.first)
					{
						auto it = structCounts.find(entityName);
						if (it != structCounts.end())
						{
							playerEntityTypeCount += it->second;
						}
					}
					minStructTypeCount = std::min(minStructTypeCount.value_or(playerEntityTypeCount), playerEntityTypeCount);
				}
				*resultMapping.pPerPlayerCount += minStructTypeCount.value_or(0);
			}

			// For the purposes of determining balance / start equality, compare the count of each different entityName separately
			for (const auto& entityName : *structureTypeCheck.first)
			{
				if (!(*resultMapping.pBalance))
				{
					break;
				}

				*resultMapping.pBalance = *resultMapping.pBalance && std::equal(++playerStructCounts.begin(), playerStructCounts.end(), playerStructCounts.begin(), [entityName](const decltype(playerStructCounts)::value_type & a, const decltype(playerStructCounts)::value_type & b) -> bool {
					auto it_a = a.find(entityName);
					auto it_b = b.find(entityName);
					uint32_t a_value = 0;
					uint32_t b_value = 0;
					if (it_a != a.end())
					{
						a_value = it_a->second;
					}
					if (it_b != b.end())
					{
						b_value = it_b->second;
					}
					return a_value == b_value;
				});
			}
		}
	}

	for (auto& feature : *pFeatures)
	{
		if (statsConfig.oilResources.count(feature.name) > 0)
		{
			results.oilWellsTotal++;
		}
	}

	return results;
}

MapStatsConfiguration::MapStatsConfiguration()
{
	// Default values extracted from WZ 4.3 stats files:

	// [DROID TEMPLATES]:
	// the names of any droid templates that are constructor droids
	//	- "type": "CONSTRUCT"
	constructorDroids = {"BabaPickUp", "CobraHoverTruck", "CobraSpadeTracks", "ConstructionDroid", "ConstructorDroid", "MantisHoverTruck", "P0CobraSpadeTracks", "P0cam3CobCONTrk", "ScorpHoverTruck"};

	// [STRUCTS]:
	// the names (ids) of resource extractor structs
	//	- "type": "RESOURCE EXTRACTOR"
	resourceExtractors = {"A0ResourceExtractor"};
	// the names (ids) of power generator structs
	//	- "type": "POWER GENERATOR"
	powerGenerators = {"A0BaBaPowerGenerator", "A0PowerGenerator"};
	// the names (ids) of factory structs
	//	- "type": "FACTORY"
	factories = {"A0BaBaFactory", "A0LightFactory"};
	//	- "type": "VTOL FACTORY"
	vtolFactories = {"A0BaBaVtolFactory", "A0VTolFactory1"};
	//	- "type": "CYBORG FACTORY"
	cyborgFactories = {"A0CyborgFactory"};
	// the names (ids) of research structs
	//	- "type": "RESEARCH"
	researchCenters = {"A0ResearchFacility"};

	// [STRUCT MODULES]:
	//	- "type": "FACTORY MODULE"
	factoryModules = {"A0FacMod1"};
	//	- "type": "RESEARCH MODULE"
	researchModules = {"A0ResearchModule1"};
	//	- "type": "POWER MODULE"
	powerModules = {"A0PowMod1"};

	// [FEATURES]:
	//	- "type": "OIL RESOURCE"
	oilResources = {"OilResource"};
	//	- "type": "OIL DRUM"
	oilDrums = {"OilDrum"};
}

// Load stats configuration for droid templates from a `templates.json` file
bool MapStatsConfiguration::loadFromTemplatesJSON(const std::string& templatesJSONPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	auto loadedResult = loadJsonObjectFromFile(templatesJSONPath, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return false;
	}

	nlohmann::json& mRoot = loadedResult.value();

	std::unordered_set<std::string> constructorDroids_loaded;
	for (const auto& it : mRoot.items())
	{
		auto type_it = it.value().find("type");
		if (type_it == it.value().end())
		{
			continue;
		}
		if (!type_it->is_string())
		{
			// Invalid type value - should be a string!
			continue;
		}
		try {
			auto typeStr = type_it->get<std::string>();
			if (typeStr.compare("CONSTRUCT") == 0)
			{
				auto& templateID = it.key();
				constructorDroids_loaded.insert(templateID);
			}
		} catch (std::exception&) {
			continue;
		}
	}

	if (!constructorDroids_loaded.empty())
	{
		constructorDroids = std::move(constructorDroids_loaded);
		return true;
	}

	return false;
}

// Load stats configuration for structures from a `structure.json` file
bool MapStatsConfiguration::loadFromStructureJSON(const std::string& structureJSONPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	auto loadedResult = loadJsonObjectFromFile(structureJSONPath, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return false;
	}

	nlohmann::json& mRoot = loadedResult.value();

	// [STRUCTS]:
	std::unordered_set<std::string> resourceExtractors_loaded;
	std::unordered_set<std::string> powerGenerators_loaded;
	std::unordered_set<std::string> factories_loaded;
	std::unordered_set<std::string> vtolFactories_loaded;
	std::unordered_set<std::string> cyborgFactories_loaded;
	std::unordered_set<std::string> researchCenters_loaded;

	// [STRUCT MODULES]:
	std::unordered_set<std::string> factoryModules_loaded;
	std::unordered_set<std::string> researchModules_loaded;
	std::unordered_set<std::string> powerModules_loaded;


	std::unordered_map<std::string, std::unordered_set<std::string>*> typeInsertionMap = {
		{ "RESOURCE EXTRACTOR", &resourceExtractors_loaded },
		{ "POWER GENERATOR", &powerGenerators_loaded },
		{ "FACTORY", &factories_loaded },
		{ "VTOL FACTORY", &vtolFactories_loaded },
		{ "CYBORG FACTORY", &cyborgFactories_loaded },
		{ "RESEARCH", &researchCenters_loaded },
		{ "FACTORY MODULE", &factoryModules_loaded },
		{ "RESEARCH MODULE", &researchModules_loaded },
		{ "POWER MODULE", &powerModules_loaded }
	};

	for (const auto& it : mRoot.items())
	{
		auto type_it = it.value().find("type");
		if (type_it == it.value().end())
		{
			continue;
		}
		if (!type_it->is_string())
		{
			// Invalid type value - should be a string!
			continue;
		}
		try {
			auto typeStr = type_it->get<std::string>();
			auto map_it = typeInsertionMap.find(typeStr);
			if (map_it != typeInsertionMap.end())
			{
				map_it->second->insert(it.key());
			}
		} catch (std::exception&) {
			continue;
		}
	}

	if (std::any_of(typeInsertionMap.begin(), typeInsertionMap.end(), [](const decltype(typeInsertionMap)::value_type & kvp) -> bool {
		return !kvp.second->empty();
	}))
	{
		resourceExtractors = std::move(resourceExtractors_loaded);
		powerGenerators = std::move(powerGenerators_loaded);
		factories = std::move(factories_loaded);
		vtolFactories = std::move(vtolFactories_loaded);
		cyborgFactories = std::move(cyborgFactories_loaded);
		researchCenters = std::move(researchCenters_loaded);
		factoryModules = std::move(factoryModules_loaded);
		researchModules = std::move(researchModules_loaded);
		powerModules = std::move(powerModules_loaded);
		return true;
	}

	return false;
}

// Load stats configuration for features from a `features.json` file
bool MapStatsConfiguration::loadFromFeaturesJSON(const std::string& featuresJSONPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	auto loadedResult = loadJsonObjectFromFile(featuresJSONPath, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return false;
	}

	nlohmann::json& mRoot = loadedResult.value();

	std::unordered_set<std::string> oilResources_loaded;
	std::unordered_set<std::string> oilDrums_loaded;
	for (const auto& it : mRoot.items())
	{
		auto type_it = it.value().find("type");
		if (type_it == it.value().end())
		{
			continue;
		}
		if (!type_it->is_string())
		{
			// Invalid type value - should be a string!
			continue;
		}
		try {
			auto typeStr = type_it->get<std::string>();
			if (typeStr.compare("OIL RESOURCE") == 0)
			{
				oilResources_loaded.insert(it.key());
			}
			else if (typeStr.compare("OIL DRUM") == 0)
			{
				oilDrums_loaded.insert(it.key());
			}
		} catch (std::exception&) {
			continue;
		}
	}

	if (!oilResources_loaded.empty() || !oilDrums_loaded.empty())
	{
		oilResources = std::move(oilResources_loaded);
		oilDrums = std::move(oilDrums_loaded);
		return true;
	}

	return false;
}

} // namespace WzMap
