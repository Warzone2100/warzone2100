
// -- definitions
const OIL_RES_STAT = "OilResource";
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
const ELECTRONIC_DEFENSES = ["Sys-SpyTower", "WallTower-EMP", "Emplacement-MortarEMP"];

// -- globals
const MIN_BASE_TRUCKS = 3;
const MIN_OIL_TRUCKS = 4;
const MIN_BUILD_POWER = 80;
const MIN_RESEARCH_POWER = -50;
const MIN_PRODUCTION_POWER = 60;
const MIN_BUSTERS = 5;
const HELP_CONSTRUCT_AREA = 20;
const AVG_BASE_RADIUS = 20; // Magic, but how will we determine the size of a "base" anyway.
const BASE = startPositions[me];
var attackGroup;
var busterGroup;
var vtolGroup;
var baseBuilders;
var oilBuilders;
var researchDone;
var truckRoleSwapped;
var isSeaMap; // Used to determine if it is a hover map.
var currentEnemy; // Current enemy SemperFi is attacking.
var currentEnemyTick; // Last time the enemy was changed.
var enemyHasVtol;

//And include the rest here.
include("/multiplay/skirmish/semperfi_includes/performance.js");
include("/multiplay/skirmish/semperfi_includes/miscFunctions.js");
include("/multiplay/skirmish/semperfi_includes/production.js");
include("/multiplay/skirmish/semperfi_includes/build.js");
include("/multiplay/skirmish/semperfi_includes/tactics.js");
include("/multiplay/skirmish/semperfi_includes/events.js");
include("/multiplay/skirmish/semperfi_includes/research.js");
include("/multiplay/skirmish/semperfi_includes/chat.js");
