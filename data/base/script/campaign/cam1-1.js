include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_scavengerRes = [
	"R-Wpn-Flamer-Damage01", "R-Wpn-MG-Damage02", "R-Wpn-MG-ROF01",
];

//Ambush player from scav base - triggered from middle path
camAreaEvent("scavBaseTrigger", function()
{
	const AMBUSH_GROUP = camMakeGroup(enumArea("eastScavsNorth", CAM_SCAV_7, false));
	camManageGroup(AMBUSH_GROUP, CAM_ORDER_ATTACK, {
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
	const AMBUSH_GROUP = camMakeGroup(enumArea("westScavs", CAM_SCAV_7, false));
	camManageGroup(AMBUSH_GROUP, CAM_ORDER_DEFEND, {
		pos: camMakePos("ambush1")
	});
}

function northwestScavAction()
{
	const AMBUSH_GROUP = camMakeGroup(enumArea("northWestScavs", CAM_SCAV_7, false));
	camManageGroup(AMBUSH_GROUP, CAM_ORDER_DEFEND, {
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

	const scavFactory = getObject("scavFactory1");
	if (camDef(scavFactory) && scavFactory && victim.type === STRUCTURE && victim.id === scavFactory.id)
	{
		camCallOnce("westScavAction");
	}
}

//Send the south-eastern scavs in the main base on an attack run when the front bunkers get destroyed.
function checkFrontBunkers()
{
	if (getObject("frontBunkerLeft") === null && getObject("frontBunkerRight") === null)
	{
		const AMBUSH_GROUP = camMakeGroup(enumArea("eastScavsSouth", CAM_SCAV_7, false));
		camManageGroup(AMBUSH_GROUP, CAM_ORDER_ATTACK, {
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

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	camCompleteRequiredResearch(mis_scavengerRes, CAM_SCAV_7);
	if (difficulty >= HARD)
	{
		completeResearch("R-Wpn-Flamer-Range01", CAM_SCAV_7);
	}

	camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, CAM_SCAV_7);
	camUpgradeOnMapTemplates(cTempl.trike, cTempl.triketwin, CAM_SCAV_7);
	camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggytwin, CAM_SCAV_7);
	camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeeptwin, CAM_SCAV_7);

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
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 40 : 30)),
			templates: [((difficulty <= MEDIUM) ? cTempl.triketwin : cTempl.trikeheavy), cTempl.blokeheavy, ((difficulty <= MEDIUM) ? cTempl.buggytwin : cTempl.buggyheavy), cTempl.bjeepheavy]
		},
	});

	camPlayVideos({video: "FLIGHT", type: CAMP_MSG});
	hackAddMessage("C1-1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	queue("checkFrontBunkers", camSecondsToMilliseconds(5));
}
