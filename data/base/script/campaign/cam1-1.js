
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

//Ambush player from scav base - triggered from middle path
camAreaEvent("scavBaseTrigger", function()
{
	const ambushGroup = camMakeGroup(enumArea("eastScavs", SCAV_7, false));
	camManageGroup(ambushGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("artifactLocation")
	});
});

//Moves west scavs units closer to the base - triggered from right path
camAreaEvent("ambush1Trigger", function()
{
	const ambushGroup = camMakeGroup(enumArea("westScavs", SCAV_7, false));
	camManageGroup(ambushGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("ambush1")
	});
});

//Sends some units towards player LZ - triggered from left path
camAreaEvent("ambush2Trigger", function()
{
	const ambushGroup = camMakeGroup(enumArea("northWestScavs", SCAV_7, false));
	camManageGroup(ambushGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("ambush2")
	});
});

function eventPickup(feature, droid)
{
	if (droid.player === CAM_HUMAN_PLAYER && feature.stattype === ARTIFACT)
	{
		hackRemoveMessage("C1-1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	}
}

//Mission setup stuff
function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_2S", {
		area: "RTLZ",
		message: "C1-1_LZ",
		reinforcements: -1, //No reinforcements
		retlz: true
	});

	const startpos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	const tent = getObject("transporterEntry");
	const text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"artifactLocation": { tech: "R-Wpn-MG3Mk1" }, //Heavy machine gun
	});

	camPlayVideos("FLIGHT");
	hackAddMessage("C1-1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

}
