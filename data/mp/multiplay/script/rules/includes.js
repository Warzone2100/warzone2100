
receiveAllEvents(true);  // If doing this in eventGameInit, it seems to be too late in T2/T3, due to some eventResearched events triggering first.

// This file contain some global variables needed for scripts included below
include("multiplay/script/rules/variables.js");

// This file contain functions which contains the logic of technology research equated to time
include("multiplay/script/functions/camTechEnabler.js");

// Spectial effects of weather.
include("multiplay/script/functions/weather.js");

/* *** SETUP *** */
//These files provide the primary match settings, and set the initial rules of the game.
include("multiplay/script/rules/setup/techlevel.js");
include("multiplay/script/rules/setup/powermodifier.js");
include("multiplay/script/rules/setup/scavengers.js");
include("multiplay/script/rules/setup/droidlimits.js");
include("multiplay/script/rules/setup/structure.js");
include("multiplay/script/rules/setup/structurelimits.js");
include("multiplay/script/rules/setup/research.js");
include("multiplay/script/rules/setup/components.js");
include("multiplay/script/rules/setup/base.js");


/* *** RULES *** */
// Displays the set match rules at the beginning of the match, for those who did not pay attention to the settings in the lobby
include("multiplay/script/rules/printsettings.js");

// Setup textures / ui
include("multiplay/script/rules/setupgame.js");

// Logic of Reticule Menu
include("multiplay/script/rules/reticule.js");

// Logic and rules of "End conditions" of the match.
include("multiplay/script/rules/endconditions.js");

// Logic of places oil barrels on the battlefield
include("multiplay/script/rules/oildrum.js");



/* *** EVENTS *** */
//Function to be executed when certain events occur in the game.
include("multiplay/script/rules/events/gameloaded.js");
include("multiplay/script/rules/events/gameinit.js");
include("multiplay/script/rules/events/attacked.js");
include("multiplay/script/rules/events/droidbuilt.js");
include("multiplay/script/rules/events/structurebuilt.js");
include("multiplay/script/rules/events/demolish.js");
include("multiplay/script/rules/events/destroyed.js");
include("multiplay/script/rules/events/transfer.js");
include("multiplay/script/rules/events/research.js");
include("multiplay/script/rules/events/upgrade.js");
include("multiplay/script/rules/events/cheat.js");
include("multiplay/script/rules/events/chat.js");

/* *** MODS *** */
//All mods, as well as modpacks, must start in this file.
//At the moment, it's an empty stub doll.
include("multiplay/script/mods/init.js");
