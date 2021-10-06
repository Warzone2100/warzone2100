
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const SCAVENGER_RES = [
	"R-Wpn-Flamer-Damage01", "R-Wpn-Flamer-Range01", "R-Wpn-MG-Damage02", "R-Wpn-MG-ROF01",
];

//Ambush player from scav base - triggered from middle path
camAreaEvent("scavBaseTrigger", function()
{
	var ambushGroup = camMakeGroup(enumArea("eastScavsNorth", SCAV_7, false));
	camManageGroup(ambushGroup, CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false
	});
});

//Moves west scavs units closer to the base - triggered from right path
camAreaEvent("ambush1Trigger", function()
{
	camCallOnce("westScavAction");
});

//Sends some units towards player LZ - triggered from left path
camAreaEvent("ambush2Trigger", function()
{
	camCallOnce("northwestScavAction");
});

camAreaEvent("factoryTrigger", function()
{
	camEnableFactory("scavFactory1");
});

function westScavAction()
{
	var ambushGroup = camMakeGroup(enumArea("westScavs", SCAV_7, false));
	camManageGroup(ambushGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("ambush1")
	});
}

function northwestScavAction()
{
	var ambushGroup = camMakeGroup(enumArea("northWestScavs", SCAV_7, false));
	camManageGroup(ambushGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("ambush2")
	});
}

function eventPickup(feature, droid)
{
	if (droid.player === CAM_HUMAN_PLAYER && feature.stattype === ARTIFACT)
	{
		hackRemoveMessage("C1-1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	}
}

function eventAttacked(victim, attacker)
{
	if (victim.player === CAM_HUMAN_PLAYER)
	{
		return;
	}
	if (!victim)
	{
		return;
	}

	if (victim.type === STRUCTURE && victim.id === 146)
	{
		camCallOnce("westScavAction");
	}
}

//Send the south-eastern scavs in the main base on an attack run when the front bunkers get destroyed.
function checkFrontBunkers()
{
	if (getObject("frontBunkerLeft") === null && getObject("frontBunkerRight") === null)
	{
		var ambushGroup = camMakeGroup(enumArea("eastScavsSouth", SCAV_7, false));
		camManageGroup(ambushGroup, CAM_ORDER_ATTACK, {
			count: -1,
			regroup: false
		});
	}
	else
	{
		queue("checkFrontBunkers", camSecondsToMilliseconds(5));
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

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camCompleteRequiredResearch(SCAVENGER_RES, SCAV_7);

	camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.trike, cTempl.triketwin, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggytwin, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeeptwin, SCAV_7);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"scavFactory1": { tech: "R-Wpn-MG3Mk1" }, //Heavy machine gun
	});

	camSetFactories({
		"scavFactory1": {
			assembly: "Assembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty === EASY || difficulty === MEDIUM) ? 40 : 30)),
			templates: [ ((difficulty === EASY || difficulty === MEDIUM) ? cTempl.triketwin : cTempl.trikeheavy), cTempl.blokeheavy, ((difficulty === EASY || difficulty === MEDIUM) ? cTempl.buggytwin : cTempl.buggyheavy), cTempl.bjeepheavy ]
		},
	});

	camPlayVideos({video: "FLIGHT", type: CAMP_MSG});
	hackAddMessage("C1-1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	queue("checkFrontBunkers", camSecondsToMilliseconds(5));
}
