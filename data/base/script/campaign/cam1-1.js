
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const scavs = 7; //Scav player number


//Ambush player from scav base - triggered from middle path
camAreaEvent("scavBaseTrigger", function()
{
	var ambushGroup = camMakeGroup(enumArea("eastScavs", scavs, false));
	camManageGroup(ambushGroup, CAM_ORDER_DEFEND, 
		{ pos: camMakePos("artifactLocation") }
	);
});

//Moves west scavs units closer to the base - triggered from right path
camAreaEvent("ambush1Trigger", function()
{
	var ambushGroup = camMakeGroup(enumArea("westScavs", scavs, false));
	camManageGroup(ambushGroup, CAM_ORDER_DEFEND, 
		{ pos: camMakePos("ambush1") }
	);
});

//Sends some units towards player LZ - triggered from left path
camAreaEvent("ambush2Trigger", function()
{
	var ambushGroup = camMakeGroup(enumArea("northWestScavs", scavs, false));
	camManageGroup(ambushGroup, CAM_ORDER_ATTACK, 
		{ pos: camMakePos("ambush2") }
	);
});

function ambushPlayerStart()
{
	var ambushGroup = camMakeGroup(enumArea("ambushScavs", scavs, false));
	camManageGroup(ambushGroup, CAM_ORDER_ATTACK, 
		{ pos: camMakePos("RTLZ") }
	);
}

//Mission setup stuff
function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_2S", {
		area: "RTLZ",
		message: "C1-1_LZ",
		reinforcements: -1 //No reinforcements
	});

	var startpos = getObject("startPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("landingZone"); //player lz
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	var tent = getObject("transporterEntry");
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	var text = getObject("transporterExit");
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"artifactLocation": { tech: "R-Wpn-MG3Mk1" }, //Heavy machine gun
	});

	hackAddMessage("FLIGHT", MISS_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C1-1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("ambushPlayerStart", 30000);
}
