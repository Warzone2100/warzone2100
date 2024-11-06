/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2023  Warzone 2100 Project

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
#include "map_jsonhelpers.h"

#include <vector>
#include <unordered_map>
#include <algorithm>

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

struct TileInfo {
	Object* psObj = nullptr;
};
struct MapTileInfo {

	MapTileInfo(uint32_t mapWidth, uint32_t mapHeight)
	: tileInfo(mapWidth * mapHeight)
	, mapWidth(mapWidth)
	, mapHeight(mapHeight)
	{ }

	std::vector<TileInfo> tileInfo;
	uint32_t mapWidth = 0;
	uint32_t mapHeight = 0;
};

static bool addObjectToTile(MapTileInfo& mapTiles, Object* obj)
{
	int32_t mapPosX = map_coord(obj->position.x);
	int32_t mapPosY = map_coord(obj->position.y);
	if (mapPosX < 0 || static_cast<uint32_t>(mapPosX) > mapTiles.mapWidth)
	{
		return false;
	}
	if (mapPosY < 0 || static_cast<uint32_t>(mapPosY) > mapTiles.mapHeight)
	{
		return false;
	}
	TileInfo& tileInfo = mapTiles.tileInfo[mapPosX + (mapPosY * mapTiles.mapWidth)];
	if (tileInfo.psObj != nullptr)
	{
		return false;
	}
	tileInfo.psObj = obj;
	return true;
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

	// Used to attempt to de-dupe oil derricks (resource extractor structures) and oil wells (oil resource features)
	//  - If an oil derrick structure is placed on the map, but there is no oil well feature beneath, it is still converted to an oil well when destroyed - including if starting at no bases.
	//	- So to get a more accurate count of oilWells on a map (as treated by the game), count tiles that have an oil well feature or resource extractor structure.
	MapTileInfo oilResourceTileObjects(results.mapWidth, results.mapHeight);

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
	optional<uint32_t> minUnitsCount;
	optional<uint32_t> maxUnitsCount;
	for (const auto& unitCounts : playerUnitCounts)
	{
		uint32_t playerUnitCount = 0;
		for (const auto& unitCount : unitCounts)
		{
			playerUnitCount += unitCount.second;
		}
		minUnitsCount = std::min(minUnitsCount.value_or(playerUnitCount), playerUnitCount);
		maxUnitsCount = std::min(maxUnitsCount.value_or(playerUnitCount), playerUnitCount);
	}
	results.perPlayerCounts.unitsPerPlayer.min = minUnitsCount.value_or(0);
	results.perPlayerCounts.unitsPerPlayer.max = maxUnitsCount.value_or(0);

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
				if (addObjectToTile(oilResourceTileObjects, &structure))
				{
					results.scavengerResourceExtractors++;
				}
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
			bool countStructure = true;
			if (statsConfig.resourceExtractors.count(structure.name) > 0)
			{
				if (!addObjectToTile(oilResourceTileObjects, &structure))
				{
					// Ignore overlapping resource extractor structure
					countStructure = false;
				}
			}
			if (countStructure)
			{
				incrementPlayerEntityCount(playerStructCounts, structure.player, structure.name);
			}
			if (statsConfig.hqStructs.count(structure.name) > 0)
			{
				// found an HQ
				std::pair<int32_t, int32_t> newHQMapPos = { map_coord(structure.position.x), map_coord(structure.position.y) };
				auto& structPlayerHQPositions = results.playerHQPositions[structure.player];
				if (std::none_of(structPlayerHQPositions.begin(), structPlayerHQPositions.end(), [newHQMapPos](const std::pair<int32_t, int32_t>& existingHQPos) -> bool {
					return existingHQPos == newHQMapPos;
				}))
				{
					structPlayerHQPositions.push_back(newHQMapPos);
				}
			}
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
	optional<uint32_t> minStructuresCount;
	optional<uint32_t> maxStructuresCount;
	for (const auto& structCounts : playerStructCounts)
	{
		uint32_t playerStructCount = 0;
		for (const auto& structCount : structCounts)
		{
			playerStructCount += structCount.second;
		}
		minStructuresCount = std::min(minStructuresCount.value_or(playerStructCount), playerStructCount);
		maxStructuresCount = std::max(maxStructuresCount.value_or(playerStructCount), playerStructCount);
	}
	results.perPlayerCounts.structuresPerPlayer.min = minStructuresCount.value_or(0);
	results.perPlayerCounts.structuresPerPlayer.max = maxStructuresCount.value_or(0);

	// Check individual types of structures
	if (!playerStructCounts.empty())
	{
		struct StructTypeMapping
		{
			bool* pBalance;
			MapStats::PerPlayerCounts::MinMax* pPerPlayerCount;
		};
		std::vector<std::pair<std::unordered_set<std::string>*, StructTypeMapping>> structureTypeChecks = {
			// resource extractors (oil derricks)
			{&statsConfig.resourceExtractors, {&results.playerBalance.resourceExtractors, &results.perPlayerCounts.resourceExtractorsPerPlayer}},
			// power generators
			{&statsConfig.powerGenerators, {&results.playerBalance.powerGenerators, &results.perPlayerCounts.powerGeneratorsPerPlayer}},
			{&statsConfig.powerModules, {&results.playerBalance.powerGenerators, &results.perPlayerCounts.powerGeneratorModulesPerPlayer}},
			// factories
			{&statsConfig.factories, {&results.playerBalance.regFactories, &results.perPlayerCounts.regFactoriesPerPlayer}},
			{&statsConfig.factoryModules, {&results.playerBalance.regFactories, &results.perPlayerCounts.regFactoryModulesPerPlayer}},
			// vtol factories
			{&statsConfig.vtolFactories, {&results.playerBalance.vtolFactories, &results.perPlayerCounts.vtolFactoriesPerPlayer}},
			// cyborg factories
			{&statsConfig.cyborgFactories, {&results.playerBalance.cyborgFactories, &results.perPlayerCounts.cyborgFactoriesPerPlayer}},
			// research centers
			{&statsConfig.researchCenters, {&results.playerBalance.researchCenters, &results.perPlayerCounts.researchCentersPerPlayer}},
			{&statsConfig.researchModules, {&results.playerBalance.researchCenters,  &results.perPlayerCounts.researchCenterModulesPerPlayer}},
			// defense structures
			{&statsConfig.defenseStructs, {&results.playerBalance.defenseStructures,  &results.perPlayerCounts.defenseStructuresPerPlayer}}
		};
		for (const auto& structureTypeCheck : structureTypeChecks)
		{
			const StructTypeMapping& resultMapping = structureTypeCheck.second;
			if (resultMapping.pPerPlayerCount)
			{
				// For the purposes of getting a count of the entity type, determine the count across all entityNames for each player
				optional<uint32_t> minStructTypeCount;
				optional<uint32_t> maxStructTypeCount;
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
					maxStructTypeCount = std::max(maxStructTypeCount.value_or(playerEntityTypeCount), playerEntityTypeCount);
				}
				resultMapping.pPerPlayerCount->min += minStructTypeCount.value_or(0);
				resultMapping.pPerPlayerCount->max += maxStructTypeCount.value_or(0);
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

	results.playerBalance.factories = results.playerBalance.regFactories && results.playerBalance.vtolFactories && results.playerBalance.cyborgFactories;

	for (auto& feature : *pFeatures)
	{
		if (statsConfig.oilResources.count(feature.name) > 0)
		{
			if (!addObjectToTile(oilResourceTileObjects, &feature))
			{
				// Ignore oil resource feature that is "on top" of existing resource extractor structure (or another oil resource feature)
				continue;
			}
		}
	}

	for (auto& tileInfo : oilResourceTileObjects.tileInfo)
	{
		if (tileInfo.psObj)
		{
			results.oilWellsTotal++;
		}
	}

	return results;
}

MapStatsConfiguration::MapStatsConfiguration(MapType mapType)
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
	// the names (ids) of the HQ struct(s)
	//  - "type": "HQ"
	//  > NOTE: Prior to WZ 4.5.4, only "A0CommandCentre" was available in multiplayer stats
	//  > With 4.5.4, the remaining campaign-only HQs were also added to MP stats
	hqStructs = {"A0CommandCentre", "A0CommandCentreNP", "A0CommandCentreCO", "A0CommandCentreNE"};
	// the names (ids) of defense structs (i.e. bunkers, towers, hardpoints)
	//  - "type": "DEFENSE"
	// extracted from the appropriate base/stats/structure.json or mp/stats/structure.json file using:
	// cat structure.json | jq '[keys[] as $k | select(.[$k].type=="DEFENSE") | $k ] | join("\", \"") as $j | "{\"" + $j + "\"}"' -r
	if (mapType == MapType::CAMPAIGN)
	{
		defenseStructs = {"A0BaBaBunker", "A0BaBaFlameTower", "A0BaBaGunTower", "A0BaBaGunTowerEND", "A0BaBaMortarPit", "A0BaBaRocketPit", "A0BaBaRocketPitAT", "A0CannonTower", "AASite-QuadBof", "AASite-QuadMg1", "AASite-QuadRotMg", "CO-Tower-HVCan", "CO-Tower-HvATRkt", "CO-Tower-HvFlame", "CO-Tower-LtATRkt", "CO-Tower-MG3", "CO-Tower-MdCan", "CO-Tower-RotMG", "CO-WallTower-HvCan", "CO-WallTower-RotCan", "CoolingTower", "Emplacement-HPVcannon", "Emplacement-Howitzer105", "Emplacement-Howitzer150", "Emplacement-HvART-pit", "Emplacement-HvyATrocket", "Emplacement-MRL-pit", "Emplacement-MdART-pit", "Emplacement-MortarPit01", "Emplacement-MortarPit02", "Emplacement-PrisLas", "Emplacement-PulseLaser", "Emplacement-Rail2", "Emplacement-Rail3", "Emplacement-Rocket06-IDF", "Emplacement-RotHow", "Emplacement-RotMor", "GuardTower-ATMiss", "GuardTower-BeamLas", "GuardTower-Rail1", "GuardTower-RotMg", "GuardTower1", "GuardTower1MG", "GuardTower2", "GuardTower3", "GuardTower3H", "GuardTower4", "GuardTower4H", "GuardTower5", "GuardTower6", "NX-CruiseSite", "NX-Emp-MedArtMiss-Pit", "NX-Emp-MultiArtMiss-Pit", "NX-Emp-Plasma-Pit", "NX-Tower-ATMiss", "NX-Tower-PulseLas", "NX-Tower-Rail1", "NX-WallTower-BeamLas", "NX-WallTower-Rail2", "NX-WallTower-Rail3", "NuclearReactor", "P0-AASite-SAM1", "P0-AASite-SAM2", "PillBox1", "PillBox2", "PillBox3", "PillBox4", "PillBox5", "PillBox6", "Pillbox-RotMG", "Sys-CB-Tower01", "Sys-NEXUSLinkTOW", "Sys-NX-CBTower", "Sys-NX-SensorTower", "Sys-NX-VTOL-CB-Tow", "Sys-NX-VTOL-RadTow", "Sys-SensoTower01", "Sys-SensoTower02", "Sys-VTOL-CB-Tower01", "Sys-VTOL-RadarTower01", "Tower-Projector", "Tower-RotMg", "Tower-VulcanCan", "UplinkCentre", "Wall-RotMg", "Wall-VulcanCan", "WallTower-Atmiss", "WallTower-HPVcannon", "WallTower-HvATrocket", "WallTower-Projector", "WallTower-PulseLas", "WallTower-Rail2", "WallTower-Rail3", "WallTower01", "WallTower02", "WallTower03", "WallTower04", "WallTower05", "WallTower06", "WreckedTransporter"};
	}
	else
	{
		defenseStructs = {"A0BaBaBunker", "A0BaBaFlameTower", "A0BaBaGunTower", "A0BaBaGunTowerEND", "A0BaBaMortarPit", "A0BaBaRocketPit", "A0BaBaRocketPitAT", "A0CannonTower", "AASite-QuadBof", "AASite-QuadBof02", "AASite-QuadMg1", "AASite-QuadRotMg", "CO-Tower-HVCan", "CO-Tower-HvATRkt", "CO-Tower-HvFlame", "CO-Tower-LtATRkt", "CO-Tower-MG3", "CO-Tower-MdCan", "CO-Tower-RotMG", "CO-WallTower-HvCan", "CO-WallTower-RotCan", "CoolingTower", "ECM1PylonMk1", "Emplacement-HPVcannon", "Emplacement-HeavyLaser", "Emplacement-HeavyPlasmaLauncher", "Emplacement-Howitzer-Incendiary", "Emplacement-Howitzer-Incenediary", "Emplacement-Howitzer105", "Emplacement-Howitzer150", "Emplacement-HvART-pit", "Emplacement-HvyATrocket", "Emplacement-MRL-pit", "Emplacement-MRLHvy-pit", "Emplacement-MdART-pit", "Emplacement-MortarEMP", "Emplacement-MortarPit-Incendiary", "Emplacement-MortarPit-Incenediary", "Emplacement-MortarPit01", "Emplacement-MortarPit02", "Emplacement-PlasmaCannon", "Emplacement-PrisLas", "Emplacement-PulseLaser", "Emplacement-Rail2", "Emplacement-Rail3", "Emplacement-Rocket06-IDF", "Emplacement-RotHow", "Emplacement-RotMor", "GuardTower-ATMiss", "GuardTower-BeamLas", "GuardTower-Rail1", "GuardTower-RotMg", "GuardTower1", "GuardTower2", "GuardTower3", "GuardTower4", "GuardTower5", "GuardTower6", "NX-CruiseSite", "NX-Emp-MedArtMiss-Pit", "NX-Emp-MultiArtMiss-Pit", "NX-Emp-Plasma-Pit", "NX-Tower-ATMiss", "NX-Tower-PulseLas", "NX-Tower-Rail1", "NX-WallTower-BeamLas", "NX-WallTower-Rail2", "NX-WallTower-Rail3", "NuclearReactor", "P0-AASite-Laser", "P0-AASite-SAM1", "P0-AASite-SAM2", "P0-AASite-Sunburst", "PillBox-Cannon6", "PillBox1", "PillBox2", "PillBox3", "PillBox4", "PillBox5", "PillBox6", "Pillbox-RotMG", "Plasmite-flamer-bunker", "Sys-CB-Tower01", "Sys-NEXUSLinkTOW", "Sys-NX-CBTower", "Sys-NX-SensorTower", "Sys-NX-VTOL-CB-Tow", "Sys-NX-VTOL-RadTow", "Sys-RadarDetector01", "Sys-SensoTower01", "Sys-SensoTower02", "Sys-SensoTowerWS", "Sys-SpyTower", "Sys-VTOL-CB-Tower01", "Sys-VTOL-RadarTower01", "Tower-Projector", "Tower-RotMg", "Tower-VulcanCan", "UplinkCentre", "Wall-RotMg", "Wall-VulcanCan", "WallTower-Atmiss", "WallTower-DoubleAAGun", "WallTower-DoubleAAGun02", "WallTower-EMP", "WallTower-HPVcannon", "WallTower-HvATrocket", "WallTower-Projector", "WallTower-PulseLas", "WallTower-QuadRotAAGun", "WallTower-Rail2", "WallTower-Rail3", "WallTower-SamHvy", "WallTower-SamSite", "WallTower-TwinAssaultGun", "WallTower01", "WallTower02", "WallTower03", "WallTower04", "WallTower05", "WallTower06", "WreckedTransporter", "bbaatow"};
	}

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

	// [STRUCT SIZES]:
	// only the data for structures that have a size != 1x1
	// extracted from the appropriate base/stats/structure.json or mp/stats/structure.json file using:
	// cat structure.json | jq 'keys[] as $k | select(.[$k].width!=1 or .[$k].breadth!=1) | "{\"" + $k + "\", StructureSize(" + (.[$k].width | tostring) + ", " + (.[$k].breadth | tostring) + ")},"' -r
	if (mapType == MapType::CAMPAIGN)
	{
		structSizes = {
			{"A0BaBaFactory", StructureSize(2, 1)},
			{"A0ComDroidControl", StructureSize(2, 2)},
			{"A0CommandCentre", StructureSize(2, 2)},
			{"A0CommandCentreCO", StructureSize(2, 2)},
			{"A0CommandCentreNE", StructureSize(2, 2)},
			{"A0CommandCentreNP", StructureSize(2, 2)},
			{"A0CyborgFactory", StructureSize(1, 2)},
			{"A0FacMod1", StructureSize(3, 3)},
			{"A0LightFactory", StructureSize(3, 3)},
			{"A0PowMod1", StructureSize(2, 2)},
			{"A0PowerGenerator", StructureSize(2, 2)},
			{"A0ResearchFacility", StructureSize(2, 2)},
			{"A0ResearchModule1", StructureSize(2, 2)},
			{"A0VTolFactory1", StructureSize(3, 3)},
			{"NuclearReactor", StructureSize(2, 2)},
			{"UplinkCentre", StructureSize(2, 2)},
			{"WreckedTransporter", StructureSize(3, 3)}
		};
	}
	else
	{
		structSizes = {
			{"A0BaBaFactory", StructureSize(2, 1)},
			{"A0BaBaVtolFactory", StructureSize(2, 2)},
			{"A0ComDroidControl", StructureSize(2, 2)},
			{"A0CommandCentre", StructureSize(2, 2)},
			{"A0CommandCentreCO", StructureSize(2, 2)},
			{"A0CommandCentreNE", StructureSize(2, 2)},
			{"A0CommandCentreNP", StructureSize(2, 2)},
			{"A0CyborgFactory", StructureSize(1, 2)},
			{"A0FacMod1", StructureSize(3, 3)},
			{"A0LasSatCommand", StructureSize(2, 2)},
			{"A0LightFactory", StructureSize(3, 3)},
			{"A0PowMod1", StructureSize(2, 2)},
			{"A0PowerGenerator", StructureSize(2, 2)},
			{"A0ResearchFacility", StructureSize(2, 2)},
			{"A0ResearchModule1", StructureSize(2, 2)},
			{"A0Sat-linkCentre", StructureSize(2, 2)},
			{"A0VTolFactory1", StructureSize(3, 3)},
			{"NuclearReactor", StructureSize(2, 2)},
			{"UplinkCentre", StructureSize(2, 2)},
			{"WreckedTransporter", StructureSize(3, 3)},
			{"X-Super-Cannon", StructureSize(2, 2)},
			{"X-Super-MassDriver", StructureSize(2, 2)},
			{"X-Super-Missile", StructureSize(2, 2)},
			{"X-Super-Rocket", StructureSize(2, 2)}
		};
	}
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
	std::unordered_set<std::string> hqStructs_loaded;
	std::unordered_set<std::string> defenseStructs_loaded;

	// [STRUCT SIZES]:
	std::unordered_map<std::string, StructureSize> structSizes_loaded;

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
		{ "HQ", &hqStructs_loaded },
		{ "DEFENSE", &defenseStructs_loaded },
		{ "FACTORY MODULE", &factoryModules_loaded },
		{ "RESEARCH MODULE", &researchModules_loaded },
		{ "POWER MODULE", &powerModules_loaded }
	};

	for (const auto& it : mRoot.items())
	{
		const auto& structId = it.key();
		StructureSize sizeInfo(1, 1);
		auto width_it = it.value().find("width");
		if (width_it != it.value().end() && width_it->is_number())
		{
			try {
				sizeInfo.baseWidth = width_it->get<uint32_t>();
			} catch (std::exception&) {
				continue;
			}
		}
		auto breadth_it = it.value().find("breadth");
		if (breadth_it != it.value().end() && breadth_it->is_number())
		{
			try {
				sizeInfo.baseBreadth = breadth_it->get<uint32_t>();
			} catch (std::exception&) {
				continue;
			}
		}
		if (sizeInfo.baseWidth > 1 || sizeInfo.baseBreadth > 1)
		{
			structSizes_loaded.insert(StructSizesMap::value_type(structId, sizeInfo));
		}

		auto type_it = it.value().find("type");
		if (type_it != it.value().end())
		{
			if (type_it->is_string())
			{
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
		}
	}

	if (std::any_of(typeInsertionMap.begin(), typeInsertionMap.end(), [](const decltype(typeInsertionMap)::value_type & kvp) -> bool {
		return !kvp.second->empty();
	}) || !structSizes_loaded.empty())
	{
		resourceExtractors = std::move(resourceExtractors_loaded);
		powerGenerators = std::move(powerGenerators_loaded);
		factories = std::move(factories_loaded);
		vtolFactories = std::move(vtolFactories_loaded);
		cyborgFactories = std::move(cyborgFactories_loaded);
		researchCenters = std::move(researchCenters_loaded);
		hqStructs = std::move(hqStructs_loaded);
		defenseStructs = std::move(defenseStructs_loaded);
		factoryModules = std::move(factoryModules_loaded);
		researchModules = std::move(researchModules_loaded);
		powerModules = std::move(powerModules_loaded);
		structSizes = std::move(structSizes_loaded);
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

optional<MapStatsConfiguration::StructureSize> MapStatsConfiguration::getStructureSize(const std::string& struct_id) const
{
	auto it = structSizes.find(struct_id);
	if (it == structSizes.end())
	{
		return nullopt;
	}
	return it->second;
}

bool MapStatsConfiguration::isStructExpansionModule(const std::string& struct_id) const
{
	return powerModules.count(struct_id)
		|| factoryModules.count(struct_id)
		|| researchModules.count(struct_id);
}

} // namespace WzMap
