
receiveAllEvents(true);  // If doing this in eventGameInit, it seems to be too late in T2/T3, due to some eventResearched events triggering first.

include("multiplay/script/rules/variables.js");

include("multiplay/script/functions/camTechEnabler.js");
include("multiplay/script/functions/weather.js");

//setup
include("multiplay/script/rules/setup/techlevel.js");
include("multiplay/script/rules/setup/powermodifier.js");
include("multiplay/script/rules/setup/scavengers.js");
include("multiplay/script/rules/setup/droidlimits.js");
include("multiplay/script/rules/setup/structure.js");
include("multiplay/script/rules/setup/structurelimits.js");
include("multiplay/script/rules/setup/research.js");
include("multiplay/script/rules/setup/components.js");
include("multiplay/script/rules/setup/base.js");


//rules
include("multiplay/script/rules/printsettings.js");
include("multiplay/script/rules/setupgame.js");
include("multiplay/script/rules/reticule.js");
include("multiplay/script/rules/endconditions.js");
include("multiplay/script/rules/oildrum.js");

// Events
include("multiplay/script/rules/events/gameloaded.js");
include("multiplay/script/rules/events/gameinit.js");
include("multiplay/script/rules/events/attacked.js");
include("multiplay/script/rules/events/droidbuilt.js");
include("multiplay/script/rules/events/structurebuilt.js");
include("multiplay/script/rules/events/demolish.js");
include("multiplay/script/rules/events/destroyed.js");
include("multiplay/script/rules/events/transfer.js");
include("multiplay/script/rules/events/research.js");
include("multiplay/script/rules/events/cheat.js");
include("multiplay/script/rules/events/chat.js");

// Mods
include("multiplay/script/mods/init.js");
