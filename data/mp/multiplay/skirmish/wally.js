// SPDX-License-Identifier: GPL-2.0-or-later

// Wally.js - Warzone 2100 AI Bot

// Author:
// Stefano Brandi <https://github.com/StefanoB88>

// License:
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// You must keep this notice to preserve authorship.

// -- definitions
const DORDER_NONE = 0;
const OIL_RES_STAT = "OilResource";
const OIL_DRUM_STAT = "OilDrum";
const ARTIFACT_STAT = "Crate";
const RES_LAB_STAT = "A0ResearchFacility";
const POW_GEN_STAT = "A0PowerGenerator";
const FACTORY_STAT = "A0LightFactory";
const DERRICK_STAT = "A0ResourceExtractor";
const CYBORG_FACTORY_STAT = "A0CyborgFactory";
const PLAYER_HQ_STAT = "A0CommandCentre";
const VTOL_PAD_STAT = "A0VtolPad";
const VTOL_FACTORY_STAT = "A0VTolFactory1";
const REPAIR_FACILITY_STAT = "A0RepairCentre3";
const SENSOR_TOWERS = ["Sys-SensoTower02", "Sys-SensoTowerWS"];
const UPLINK_STAT = "A0Sat-linkCentre";
const LASERLINK_STAT = "A0LasSatCommand";
const ELECTRONIC_DEFENSES = ["Sys-SpyTower", "WallTower-EMP", "Emplacement-MortarEMP"];

// -- globals
const MAX_BASE_TRUCKS = 12;
const MIN_TRUCKS_TO_KEEP = 3; // Trucks Wally always keeps for itself before donating one to an ally
const WALLY_DEBUG = false; // Set to true to log slow ticks and ally-support decisions to the debug console
const MIN_BUILD_POWER = 30;
const MIN_PRODUCTION_POWER = 60;
const BASE_THREAT_RANGE = Math.ceil(17 + (mapWidth + mapHeight) / 2 / 20);
var BASE = startPositions[me];
var baseBuildersGroup;
var oilGrabbersGroup;
var defensesBuildersGroup;
var attackerGroup;
var bunkerBusterGroup;
var helperGroup;
var vtolGroup;
var aaGroup; // Mobile anti-air tanks: escort the army instead of leading it
var rallyGroup; // Fresh reinforcements waiting at base to join the front as a batch
var researchDone;
var currentlyDead;
var isSeaMap; // Used to determine if it is a hover map.
var isMyBaseInTrouble = false; // Used to determine if the base is in danger.
var isHighOilMap;
var mapOilPerPlayer; // Oil resources per player on this map, measured once at game start
var enemiesHaveVTOLs = false; // Used to determine if VTOLs are allowed.
var vtolsEnabled = true; // False when the VTOL factory is disabled in the game limits (no AA is researched or built then)
var lastVtolStrike; // {x, y, time} of the last enemy VTOL hit on our stuff, used to position anti-air
var areAllEnemiesDead = false;
var isHelpingAlly = false;

// Returns the desired truck count based on current economy (Cobra strategy: minimum 6 trucks on low/normal oil, scaling up with derricks)
function getDesiredTruckCount() {
	const derricks = countStruct(DERRICK_STAT);
	if (isHighOilMap || derricks >= 20) return 11;
	if (derricks >= 14) return 9;
	if (derricks >= 8) return 7;
	// Once every reachable oil is taken on a small economy, 5 trucks are enough and the saved power goes to the army
	if (typeof hasFreeSafeOils !== "undefined" && !hasFreeSafeOils && gameTime > 240000) return 5;
	return 6;
}

/**
 * @typedef {Object} PlayerData
 * @property {string} difficulty - The player's difficulty level
 * @property {string} colour - The player's color
 * @property {string} position - The player's index
 * @property {boolean} isAI - Whether the player is AI-controlled
 * @property {boolean} isHuman - Whether the player is human
 * @property {string} name - The player's name
 * @property {number} team - The player's team
 * @property {{x: number, y: number}} base - The player's base coordinates
 */

/** @type {PlayerData[]} */
var alliesList = [];
var currentHelpedAlly;

/** @type {PlayerData} */
var currentEnemy; // Current enemy Wally is attacking.

var scavengerIndex = (typeof scavengers !== "undefined" && scavengers && typeof scavengerPlayer !== "undefined" && scavengerPlayer >= 0) ? scavengerPlayer : undefined;

// Map of dangerous coordinates {"x_y": expirationGameTime} where our units/derricks were recently attacked
var dangerZones = new Map();

// Tracks pending build orders per truck {droidId -> {stat, x, y, time}} so trucks don't duplicate the same building while walking
var pendingBuildOrders = new Map();

// Tracks how long each truck has been stationary/idle {droidId -> {x, y, idleSince}} to recall trucks stuck/idle > 60s
var truckIdleTracker = new Map();

// Difficulty
var reactionSpeedDelay = 0;

// And include the rest here.
include("/multiplay/skirmish/wally_includes/start.js");
include("/multiplay/skirmish/wally_includes/functionsUtility.js");
include("/multiplay/skirmish/wally_includes/production.js");
include("/multiplay/skirmish/wally_includes/productionUtility.js");
include("/multiplay/skirmish/wally_includes/productionTactics.js");
include("/multiplay/skirmish/wally_includes/research.js");
include("/multiplay/skirmish/wally_includes/researchUtility.js");
include("/multiplay/skirmish/wally_includes/build.js");
include("/multiplay/skirmish/wally_includes/buildUtility.js");
include("/multiplay/skirmish/wally_includes/attack.js");
include("/multiplay/skirmish/wally_includes/help.js");
include("/multiplay/skirmish/wally_includes/chat.js");
include("/multiplay/skirmish/wally_includes/laserlink.js");
