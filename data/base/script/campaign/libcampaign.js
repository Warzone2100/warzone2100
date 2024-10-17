//;; # `libcampaign.js` documentation
//;;
//;; `libcampaign.js` is a JavaScript library supplied with the game,
//;; which contains reusable code for campaign scenarios. It is designed to
//;; make scenario development as high-level and declarative as possible.
//;; It also contains a few simple convenient wrappers.
//;; Public API functions of `libcampaign.js` are prefixed with `cam`.
//;; To use `libcampaign.js`, add the following include into your scenario code:
//;;
//;; ```js
//;; include("script/campaign/libcampaign.js");
//;; ```
//;;
//;; Also, most of the `libcampaign.js` features require some of the game
//;; events handled by the library. Transparent JavaScript pre-hooks are
//;; therefore injected into your global event handlers upon include.
//;; For example, if `camSetArtifacts()` was called to let `libcampaign.js`
//;; manage scenario artifacts, then `eventPickup()` will be first handled
//;; by the library, and only then your handler will be called, if any.
//;; All of this happens automagically and does not normally require
//;; your attention.
//;;

/*
	Private vars and functions are prefixed with `__cam`.
	Private consts are prefixed with `__CAM_` or `__cam_`.
	Public vars/functions are prefixed with `cam`, consts with `CAM_` or `cam_`.
	Please do not use private stuff in scenario code, use only public API.

	It is encouraged to prefix any local consts with `__` in any function in the
	library if they are not objects/arrays. Mission scripts may use a `_` but
	only if the name seems like it could clash with a JS API global.

	Please CAPITALIZE const names for consistency for most of everything.
	The only exception to these rules is when the const is declared in a loop
	initialization or will be assigned as a global-context callback function,
	or if it will be a JS object/array as these aren't truly immutable. Follow
	standard camel case style as usual.

	Also, in the event you want a top level const for a mission script
	(and any include file) please prefix it with `MIS_` or `mis_` depending on
	if it's an object/array or not.

	We CANNOT put our private vars into an anonymous namespace, even though
	it's a common JS trick -

		(function(global) {
			var __camPrivateVar; // something like that
		})(this);

	because they would break on savegame-loadgame. So let's just agree
	that we'd never use them anywhere outside the file, so that it'd be easier
	to change them, and also think twice before we change the public API.

	The lib is split into sections, each section is separated with a slash line:

////////////////////////////////////////////////////////////////////////////////
// Section name.
////////////////////////////////////////////////////////////////////////////////

	yeah, like that. Also, it's exactly 80 characters wide.

	In each section, public stuff is on TOP, and private stuff
	is below, split from the public stuff with:

//////////// privates

	, for easier reading (all the useful stuff on top).

	Please leave camDebug() around if something that should never happen
	occurs, indicating an actual bug in campaign. Then a sensible message
	should be helpful as well. But normally, no warnings should ever be
	printed.

	In cheat mode, more warnings make sense, explaining what's going on
	under the hood. Use camTrace() for such warnings - they'd auto-hide
	outside cheat mode.

	Lines prefixed with // followed by ;; are docstrings for JavaScript API
	documentation.
*/

////////////////////////////////////////////////////////////////////////////////
// Library initialization.
////////////////////////////////////////////////////////////////////////////////

// Registers a private event namespace for this library, to avoid collisions with
// any event handling in code using this library. Make sure no other library uses
// the same namespace, or strange things will happen. After this, we can name our
// event handlers with the registered prefix, and they will still get called.
namespace("cam_");

//////////global vars start

var __camClassicModActive = (modList.indexOf("wz2100_camclassic.wz") !== -1);

//These are campaign player numbers.
const CAM_HUMAN_PLAYER = 0;
const CAM_NEW_PARADIGM = 1;
const CAM_THE_COLLECTIVE = 2;
const CAM_NEXUS = 3;
const CAM_SCAV_6 = 6;
const CAM_SCAV_7 = 7;

const __CAM_MAX_PLAYERS = 8;
const __CAM_TICKS_PER_FRAME = 100;
const __CAM_AI_POWER = 999999;
const __CAM_INCLUDE_PATH = "script/campaign/libcampaign_includes/";

//Anything stats related
const CAM_ARTIFACT_STAT = "Crate";
const CAM_GENERIC_TRUCK_STAT = "Spade1Mk1";
const CAM_GENERIC_LAND_STAT = "wheeled01";
const CAM_GENERIC_WATER_STAT = "hover01";
const CAM_OIL_RESOURCE_STAT = "OilResource";
const cam_base_structures = {
	commandRelay: "A0ComDroidControl",
	commandCenter: "A0CommandCentre",
	powerGenerator: "A0PowerGenerator",
	researchLab: "A0ResearchFacility",
	factory: "A0LightFactory",
	derrick: "A0ResourceExtractor"
};
const cam_resistance_circuits = {
	first: "R-Sys-Resistance-Upgrade01",
	second: "R-Sys-Resistance-Upgrade02",
	third: "R-Sys-Resistance-Upgrade03",
	fourth: "R-Sys-Resistance-Upgrade04"
};

//level load codes here for reference. Might be useful for later code.
const CAM_GAMMA_OUT = "GAMMA_OUT"; //Fake next level for the final Gamma mission.
const __CAM_ALPHA_CAMPAIGN_NUMBER = 1;
const __CAM_BETA_CAMPAIGN_NUMBER = 2;
const __CAM_GAMMA_CAMPAIGN_NUMBER = 3;
const __CAM_UNKNOWN_CAMPAIGN_NUMBER = 1000;
const cam_levels = {
	alpha1: "CAM_1A",
	alpha2: "CAM_1B",
	alpha3: {pre: "SUB_1_1S", offWorld: "SUB_1_1"},
	alpha4: {pre: "SUB_1_2S", offWorld: "SUB_1_2"},
	alpha5: {pre: "SUB_1_3S", offWorld: "SUB_1_3"},
	alpha6: "CAM_1C",
	alpha7: "CAM_1CA",
	alpha8: {pre: "SUB_1_4AS", offWorld: "SUB_1_4A"},
	alpha9: {pre: "SUB_1_5S", offWorld: "SUB_1_5"},
	alpha10: "CAM_1A-C",
	alpha11: {pre: "SUB_1_7S", offWorld: "SUB_1_7"},
	alpha12: {pre: "SUB_1_DS", offWorld: "SUB_1_D"},
	alphaEnd: "CAM_1END",
	beta1: "CAM_2A",
	beta2: {pre: "SUB_2_1S", offWorld: "SUB_2_1"},
	beta3: "CAM_2B",
	beta4: {pre: "SUB_2_2S", offWorld: "SUB_2_2"},
	beta5: "CAM_2C",
	beta6: {pre: "SUB_2_5S", offWorld: "SUB_2_5"},
	beta7: {pre: "SUB_2DS", offWorld: "SUB_2D"},
	beta8: {pre: "SUB_2_6S", offWorld: "SUB_2_6"},
	beta9: {pre: "SUB_2_7S", offWorld: "SUB_2_7"},
	beta10: {pre: "SUB_2_8S", offWorld: "SUB_2_8"},
	betaEnd: "CAM_2END",
	gamma1: "CAM_3A",
	gamma2: {pre: "SUB_3_1S", offWorld: "SUB_3_1"},
	gamma3: "CAM_3B",
	gamma4: {pre: "SUB_3_2S", offWorld: "SUB_3_2"},
	gamma5: "CAM3A-B",
	gamma6: "CAM3C",
	gamma7: "CAM3A-D1",
	gamma8: "CAM3A-D2",
	gammaEnd: {pre: "CAM_3_4S", offWorld: "CAM_3_4"}
};
const __cam_alphaLevels = [
	cam_levels.alpha1, cam_levels.alpha2, cam_levels.alpha3.pre, cam_levels.alpha3.offWorld,
	cam_levels.alpha4.pre, cam_levels.alpha4.offWorld, cam_levels.alpha5.pre,
	cam_levels.alpha5.offWorld, cam_levels.alpha6, cam_levels.alpha7, cam_levels.alpha8.pre,
	cam_levels.alpha8.offWorld, cam_levels.alpha9.pre, cam_levels.alpha9.offWorld,
	cam_levels.alpha10, cam_levels.alpha11.pre, cam_levels.alpha11.offWorld,
	cam_levels.alpha12.pre, cam_levels.alpha12.offWorld, cam_levels.alphaEnd
];
const __cam_betaLevels = [
	cam_levels.beta1, cam_levels.beta2.pre, cam_levels.beta2.offWorld, cam_levels.beta3,
	cam_levels.beta4.pre, cam_levels.beta4.offWorld, cam_levels.beta5, cam_levels.beta6.pre,
	cam_levels.beta6.offWorld, cam_levels.beta7.pre, cam_levels.beta7.offWorld,
	cam_levels.beta8.pre, cam_levels.beta8.offWorld, cam_levels.beta9.pre,
	cam_levels.beta9.offWorld, cam_levels.beta10.pre, cam_levels.beta10.offWorld,
	cam_levels.betaEnd
];
const __cam_gammaLevels = [
	cam_levels.gamma1, cam_levels.gamma2.pre, cam_levels.gamma2.offWorld, cam_levels.gamma3,
	cam_levels.gamma4.pre, cam_levels.gamma4.offWorld, cam_levels.gamma5, cam_levels.gamma6,
	cam_levels.gamma7, cam_levels.gamma8, cam_levels.gammaEnd.pre, cam_levels.gammaEnd.offWorld
];

// Holds all the sounds the campaign uses. Try to name things as they are said.
const cam_sounds = {
	baseDetection: {
		scavengerOutpostDetected: "pcv375.ogg",
		scavengerBaseDetected: "pcv374.ogg",
		enemyBaseDetected: "pcv379.ogg",
	},
	baseElimination: {
		scavengerOutpostEradicated: "pcv391.ogg",
		scavengerBaseEradicated: "pcv392.ogg",
		enemyBaseEradicated: "pcv394.ogg",
	},
	lz: {
		returnToLZ: "pcv427.ogg",
		LZCompromised: "pcv445.ogg",
		LZClear: "lz-clear.ogg",
	},
	transport: {
		transportUnderAttack: "pcv443.ogg",
		enemyTransportDetected: "pcv381.ogg",
		incomingEnemyTransport: "pcv395.ogg",
	},
	incoming: {
		incomingIntelligenceReport: "pcv456.ogg",
		incomingTransmission: "pcv455.ogg",
	},
	rescue: {
		unitsRescued: "pcv615.ogg",
		groupRescued: "pcv616.ogg",
		civilianRescued: "pcv612.ogg",
	},
	nexus: {
		defensesAbsorbed: "defabsrd.ogg",
		defensesNeutralized: "defnut.ogg",
		laugh1: "laugh1.ogg",
		laugh2: "laugh2.ogg",
		laugh3: "laugh3.ogg",
		productionCompleted: "pordcomp.ogg",
		researchAbsorbed: "resabsrd.ogg",
		structureAbsorbed: "strutabs.ogg",
		structureNeutralized: "strutnut.ogg",
		synapticLinksActivated: "synplnk.ogg",
		unitAbsorbed: "untabsrd.ogg",
		unitNeutralized: "untnut.ogg",
	},
	missile: {
		launch: {
			missileLaunchAborted: "labort.ogg",
			missileLaunched: "mlaunch.ogg",
			finalMissileLaunchSequenceInitiated: "flseq.ogg",
			missileEnteringFinalLaunchPeriod: "meflp.ogg",
			missileLaunchIn60Minutes: "60min.ogg",
			missileLaunchIn50Minutes: "50min.ogg",
			missileLaunchIn40Minutes: "40min.ogg",
			missileLaunchIn30Minutes: "30min.ogg",
			missileLaunchIn20Minutes: "20min.ogg",
			missileLaunchIn10Minutes: "10min.ogg",
			missileLaunchIn5Minutes: "5min.ogg",
			missileLaunchIn4Minutes: "4min.ogg",
			missileLaunchIn3Minutes: "3min.ogg",
			missileLaunchIn2Minutes: "2min.ogg",
			missileLaunchIn1Minute: "1min.ogg",
		},
		detonate: {
			warheadActivatedCountdownBegins: "wactivat.ogg",
			finalDetonationSequenceInitiated: "fdetseq.ogg",
			detonationIn60Minutes: "det60min.ogg",
			detonationIn50Minutes: "det50min.ogg",
			detonationIn40Minutes: "det40min.ogg",
			detonationIn30Minutes: "det30min.ogg",
			detonationIn20Minutes: "det20min.ogg",
			detonationIn10Minutes: "det10min.ogg",
			detonationIn5Minutes: "det5min.ogg",
			detonationIn4Minutes: "det4min.ogg",
			detonationIn3Minutes: "det3min.ogg",
			detonationIn2Minutes: "det2min.ogg",
			detonationIn1Minute: "det1min.ogg",
		},
		countdown: "10to1.ogg",
	},
	reinforcementsAreAvailable: "pcv440.ogg",
	objectiveCaptured: "pcv621.ogg",
	enemyEscaping: "pcv632.ogg",
	powerTransferred: "power-transferred.ogg",
	laserSatelliteFiring: "pcv650.ogg",
	artifactRecovered: "pcv352.ogg",
	soundIdentifier: ".ogg", //Used by video.js to check for sound before a video.
};

//artifact
var __camArtifacts;
var __camNumArtifacts;

//base
var __camEnemyBases;
var __camNumEnemyBases;

//reinforcements
const CAM_REINFORCE_NONE = 0;
const CAM_REINFORCE_GROUND = 1;
const CAM_REINFORCE_TRANSPORT = 2;
const CAM_REINFORCE_CONDITION_NONE = 0;
const CAM_REINFORCE_CONDITION_BASES = 1;
const CAM_REINFORCE_CONDITION_UNITS = 2;
const CAM_REINFORCE_CONDITION_OBJECT = 3;
const CAM_REINFORCE_CONDITION_ARTIFACTS = 4;

//debug
var __camMarkedTiles = {};
var __camCheatMode = false;
var __camDebuggedOnce = {};
var __camTracedOnce = {};

//events
var __camSaveLoading;

//group
var __camNewGroupCounter;
var __camNeverGroupDroids;

//hook
var __camOriginalEvents = {};

//misc
var __camCalledOnce = {};
var __camExpLevel;

//nexus
var __camLastNexusAttack;
var __camNexusActivated;

//production
var __camFactoryInfo;
var __camFactoryQueue;
var __camPropulsionTypeLimit;

//research
const __CAM_AI_INSTANT_PRODUCTION_RESEARCH = "R-Struc-Factory-Upgrade-AI";
const cam_nexusSpecialResearch = [
	"R-Sys-NEXUSrepair", "R-Sys-NEXUSsensor"
];

//tactics
const CAM_ORDER_ATTACK = 0;
const CAM_ORDER_DEFEND = 1;
const CAM_ORDER_PATROL = 2;
const CAM_ORDER_COMPROMISE = 3;
const CAM_ORDER_FOLLOW = 4;
var __camGroupInfo;
const __CAM_TARGET_TRACKING_RADIUS = 7;
const __CAM_PLAYER_BASE_RADIUS = 20;
const __CAM_DEFENSE_RADIUS = 4;
const __CAM_CLOSE_RADIUS = 2;
const __CAM_CLUSTER_SIZE = 4;
const __CAM_FALLBACK_TIME_ON_REGROUP = 5000;
var __camGroupAvgCoord = {x: 0, y: 0};

//time
const CAM_MILLISECONDS_IN_SECOND = 1000;
const CAM_SECONDS_IN_MINUTE = 60;
const CAM_MINUTES_IN_HOUR = 60;

//transport
const cam_trComps = {
	name: "Transport",
	body: "TransporterBody",
	propulsion: "V-Tol",
	weapon: "MG3-VTOL"
};
var __camNumTransporterExits;
var __camPlayerTransports;
var __camIncomingTransports;
var __camTransporterQueue;
var __camTransporterMessage;

//truck
var __camTruckInfo;

//victory
const CAM_VICTORY_STANDARD = "__camVictoryStandard";
const CAM_VICTORY_PRE_OFFWORLD = "__camVictoryPreOffworld";
const CAM_VICTORY_OFFWORLD = "__camVictoryOffworld";
const CAM_VICTORY_TIMEOUT = "__camVictoryTimeout";
var __camWinLossCallback;
var __camNextLevel;
var __camNeedBonusTime;
var __camDefeatOnTimeout;
var __camVictoryData;
var __camRTLZTicker;
var __camLZCompromisedTicker;
var __camLastAttackTriggered;
var __camLevelEnded;
var __camExtraObjectiveMessage;
var __camAllowVictoryMsgClear;

//video
var __camVideoSequences;

//vtol
var __camVtolDataSystem;
//////////globals vars end

// A hack to make sure we do not put this variable into the savegame. It is
// called from top level, because we need to call it again every time we load
// scripts. But other than this one, you should in general never call game
// functions from toplevel, since all game state may not be fully initialized
// yet at the time scripts are loaded. (Yes, function name needs to be quoted.)
hackDoNotSave("__camOriginalEvents");

include(__CAM_INCLUDE_PATH + "misc.js");
include(__CAM_INCLUDE_PATH + "debug.js");
include(__CAM_INCLUDE_PATH + "hook.js");
include(__CAM_INCLUDE_PATH + "events.js");

include(__CAM_INCLUDE_PATH + "time.js");
include(__CAM_INCLUDE_PATH + "research.js");
include(__CAM_INCLUDE_PATH + "artifact.js");
include(__CAM_INCLUDE_PATH + "base.js");
include(__CAM_INCLUDE_PATH + "reinforcements.js");
include(__CAM_INCLUDE_PATH + "tactics.js");
include(__CAM_INCLUDE_PATH + "production.js");
include(__CAM_INCLUDE_PATH + "truck.js");
include(__CAM_INCLUDE_PATH + "victory.js");
include(__CAM_INCLUDE_PATH + "transport.js");
include(__CAM_INCLUDE_PATH + "vtol.js");
include(__CAM_INCLUDE_PATH + "nexus.js");
include(__CAM_INCLUDE_PATH + "group.js");
include(__CAM_INCLUDE_PATH + "video.js");
include(__CAM_INCLUDE_PATH + "guide.js");
