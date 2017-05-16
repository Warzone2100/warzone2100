
//TODO: The WZ script SmepferFi has a scout group that patrols around the map
//and defense structures are build near derricks. Repair cyborgs/units were made as well.
//It should be able to outpace Nexus in grabbing oil.

// -- definitions
const oilRes = "OilResource";
const resLab = "A0ResearchFacility";
const powGen = "A0PowerGenerator";
const factory = "A0LightFactory";
const derrick = "A0ResourceExtractor";
const cybFactory = "A0CyborgFactory";
const playerHQ = "A0CommandCentre";
const vtolPad = "A0VtolPad";
const vtolFactory = "A0VTolFactory1";
const cbTower = "Sys-CB-Tower01";
const wideSpecTower = "Sys-SensoTowerWS";

// -- globals
var attackGroup;
var vtolGroup;
var researchDone = false;
var isSeaMap; // Used to determine if it is a hover map.
var lastBeaconDrop = 0; // last game time SemperFi sent a beacon.
var currentEnemy; // Current enemy SemperFi is attacking.
var currentEnemyTick; // Last time the enemy was changed.

//And include the rest here.
include("/multiplay/skirmish/semperfi_includes/miscFunctions.js");
include("/multiplay/skirmish/semperfi_includes/production.js");
include("/multiplay/skirmish/semperfi_includes/build.js");
include("/multiplay/skirmish/semperfi_includes/tactics.js");
include("/multiplay/skirmish/semperfi_includes/events.js");
include("/multiplay/skirmish/semperfi_includes/research.js");
include("/multiplay/skirmish/semperfi_includes/chat.js");
