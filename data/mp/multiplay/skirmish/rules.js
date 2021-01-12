//
// Skirmish Base Script.
//
// contains the rules for starting and ending a game.
// as well as warning messages.
//
// /////////////////////////////////////////////////////////////////

receiveAllEvents(true);  // If doing this in eventGameInit, it seems to be too late in T2/T3, due to some eventResearched events triggering first.

include("multiplay/script/variables.js");

include("multiplay/script/camTechEnabler.js");
include("multiplay/script/weather.js");

//rules
include("multiplay/script/rules/printsettings.js");
include("multiplay/script/rules/setupgame.js");
include("multiplay/script/rules/reticule.js");
include("multiplay/script/rules/endconditions.js");
include("multiplay/script/rules/oildrum.js");

// Events
include("multiplay/script/events/gameloaded.js");
include("multiplay/script/events/gameinit.js");
include("multiplay/script/events/attacked.js");
include("multiplay/script/events/droidbuilt.js");
include("multiplay/script/events/structurebuilt.js");
include("multiplay/script/events/demolish.js");
include("multiplay/script/events/destroyed.js");
include("multiplay/script/events/transfer.js");
include("multiplay/script/events/research.js");
include("multiplay/script/events/cheat.js");
include("multiplay/script/events/chat.js");

// Mods
include("multiplay/script/mods/init.js");
