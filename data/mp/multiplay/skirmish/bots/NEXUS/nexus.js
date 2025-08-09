
////////////////////////////////////////////////////////////////////////////////
// Nexus AI - the Javascript translation from the wzscript form.
//
// Rules to follow with this script:
//
// * An essential rule found among these scripts are to avoid passing game objects,
// but rather their ID mostly. This helps reduce a measurable performance impact.
////////////////////////////////////////////////////////////////////////////////

const NEXUS_INCLUDES = "multiplay/skirmish/nexus_includes/";
const NEXUS_STANDARDS = "multiplay/skirmish/nexus_standards/";
const MAX_DROID_LIMIT = getDroidLimit();

const BASE_DEFEND_DURATION = (3 * 60000);
const HELP_REQUEST_INTERVAL = (3 * 60000);
const BEACON_TIMEOUT = 30000;

const LOW_POWER = 140; //250
const NEARBY_OIL_SCAN_RADIUS = 10;
const OIL_THREAT_RADIUS = 7;

const TRUCK_INFO = {
	min: 4, // 5
	max: 7, // 12
};

const SCAN_RANGE_FOR_AA = 12; //Check if an enemy structure is surrounded by AA. If too many then ignore it.
const BASE_DEFENSE_RANGE = 40; //How far away from the farthest base structure we can go.
const BASE_VTOL_DEFENSE_RANGE = 25; //see above.
const BASE_THREAT_RANGE = Math.ceil(17 + (mapWidth + mapHeight) / 2 / 35);

const TEMPLATE_BEST_OFFSET = 5; //Choose the best and the previous five to choose from.

const TICK = 100;

const BASE = startPositions[me];
const MAP_GATES = enumGateways();
const DARK_ZONE_TILES = 2; //The "dark" area surrounding the map.

var debugMode;
var helpInfo;
var defendInfo;
var scoutInfo;
var groups;
var baseLimits;
var numVtolUnits;
var rebuildQueue;
var targetInfo;
var allianceTime;

include(NEXUS_STANDARDS + "buildDefinitions.js");
include(NEXUS_STANDARDS + "templates.js");
include(NEXUS_STANDARDS + "researchDefinitions.js");
include(NEXUS_STANDARDS + "personalityDefinitions.js");
include(NEXUS_STANDARDS + "chatDefinitions.js");

//files with events first as a matter of personal preference
include(NEXUS_INCLUDES + "misc.js");
include(NEXUS_INCLUDES + "events.js");
include(NEXUS_INCLUDES + "chat.js");
include(NEXUS_INCLUDES + "alliance.js");
include(NEXUS_INCLUDES + "research.js");
//And everything else
include(NEXUS_INCLUDES + "debug.js");
include(NEXUS_INCLUDES + "build.js");
include(NEXUS_INCLUDES + "enemySelection.js");
include(NEXUS_INCLUDES + "help.js");
include(NEXUS_INCLUDES + "lassat.js");
include(NEXUS_INCLUDES + "personality.js");
include(NEXUS_INCLUDES + "production.js");
include(NEXUS_INCLUDES + "scouting.js");
include(NEXUS_INCLUDES + "tactics.js");
include(NEXUS_INCLUDES + "vtolTactics.js");
