//;; # libcampaign.js documentation
//;;
//;; ```libcampaign.js``` is a JavaScript library supplied with the game,
//;; which contains reusable code for campaign scenarios. It is designed to
//;; make scenario development as high-level and declarative as possible.
//;; It also contains a few simple convenient wrappers.
//;; Public API functions of ```libcampaign.js``` are prefixed with
//;; ```cam```. To use ```libcampaign.js```, add the following include
//;; into your scenario code:
//;;
//;; ```javascript
//;; include("script/campaign/libcampaign.js");
//;; ```
//;;
//;; Also, most of the ```libcampaign.js``` features require some of the
//;; game events handled by the library. Transparent JavaScript pre-hooks are
//;; therefore injected into your global event handlers upon include. For
//;; example, if ```camSetArtifacts()``` was called to let
//;; ```libcampaign.js``` manage scenario artifacts, then
//;; ```eventPickup()``` will be first handled by the library, and only then
//;; your handler will be called, if any.
//;; All of this happens automagically and does not normally require
//;; your attention.
//;;

/*
	Private vars and functions are prefixed with `__cam'.
	Please do not use private stuff in scenario code, use only public API.

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
//These are campaign player numbers.
const CAM_HUMAN_PLAYER = 0;
const NEW_PARADIGM = 1;
const THE_COLLECTIVE = 2;
const NEXUS = 3;
const SCAV_6 = 6;
const SCAV_7 = 7;

const CAM_MAX_PLAYERS = 8;
const CAM_TICKS_PER_FRAME = 100;
const AI_POWER = 999999;
const INCLUDE_PATH = "script/campaign/libcampaign_includes/";

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

//debug
var __camMarkedTiles = {};
var __camCheatMode = false;
var __camDebuggedOnce = {};
var __camTracedOnce = {};

//events
var __camLastHitTime = 0;
var __camSaveLoading;

//group
var __camNewGroupCounter;
var __camNeverGroupDroids;

//hook
var __camOriginalEvents = {};

//misc
var __camCalledOnce = {};

//nexus
const DEFENSE_ABSORBED = "defabsrd.ogg";
const DEFENSE_NEUTRALIZE = "defnut.ogg";
const LAUGH1 = "laugh1.ogg";
const LAUGH2 = "laugh2.ogg";
const LAUGH3 = "laugh3.ogg";
const PRODUCTION_COMPLETE = "pordcomp.ogg";
const RES_ABSORBED = "resabsrd.ogg";
const STRUCTURE_ABSORBED = "strutabs.ogg";
const STRUCTURE_NEUTRALIZE = "strutnut.ogg";
const SYNAPTICS_ACTIVATED = "synplnk.ogg";
const UNIT_ABSORBED = "untabsrd.ogg";
const UNIT_NEUTRALIZE = "untnut.ogg";
var __camLastNexusAttack;
var __camNexusActivated;

//production
var __camFactoryInfo;
var __camFactoryQueue;
var __camPropulsionTypeLimit;

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

//time
const MILLISECONDS_IN_SECOND = 1000;
const SECONDS_IN_MINUTE = 60;
const MINUTES_IN_HOUR = 60;

//transport
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
var __camVictoryMessageThrottle;

//video
var __camVideoSequences;

//vtol
var __camVtolPlayer;
var __camVtolStartPosition;
var __camVtolTemplates;
var __camVtolExitPosition;
var __camVtolSpawnActive;
var __camVtolSpawnStopObject;
var __camVtolExtras;
//////////globals vars end

// A hack to make sure we do not put this variable into the savegame. It is
// called from top level, because we need to call it again every time we load
// scripts. But other than this one, you should in general never call game
// functions from toplevel, since all game state may not be fully initialized
// yet at the time scripts are loaded. (Yes, function name needs to be quoted.)
hackDoNotSave("__camOriginalEvents");

include(INCLUDE_PATH + "misc.js");
include(INCLUDE_PATH + "debug.js");
include(INCLUDE_PATH + "hook.js");
include(INCLUDE_PATH + "events.js");

include(INCLUDE_PATH + "time.js");
include(INCLUDE_PATH + "research.js");
include(INCLUDE_PATH + "artifact.js");
include(INCLUDE_PATH + "base.js");
include(INCLUDE_PATH + "reinforcements.js");
include(INCLUDE_PATH + "tactics.js");
include(INCLUDE_PATH + "production.js");
include(INCLUDE_PATH + "truck.js");
include(INCLUDE_PATH + "victory.js");
include(INCLUDE_PATH + "transport.js");
include(INCLUDE_PATH + "vtol.js");
include(INCLUDE_PATH + "nexus.js");
include(INCLUDE_PATH + "group.js");
include(INCLUDE_PATH + "video.js");
