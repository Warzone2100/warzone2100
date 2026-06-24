include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const MIS_UPLINK_PLAYER = 1; //The satellite uplink player number.
const mis_collectiveRes = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05", "R-Sys-Engineering02",
	"R-Vehicle-Engine05", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05", "R-Wpn-Cannon-ROF02",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc03", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage05",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF02", "R-Wpn-Howitzer-ROF02",
	"R-Wpn-Howitzer-Damage08", "R-Cyborg-Armor-Heat01", "R-Vehicle-Armor-Heat01",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage02", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy01", "R-Struc-VTOLPad-Upgrade03",
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade04", "R-Struc-Materials05", "R-Struc-Factory-Upgrade05",
	"R-Struc-VTOLPad-Upgrade03", "R-Vehicle-Engine05", "R-Vehicle-Metals05",
	"R-Cyborg-Metals05", "R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-Howitzer-Accuracy01", "R-Wpn-Howitzer-Damage01", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Wpn-RocketSlow-ROF03"
];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if ((droid.player !== CAM_HUMAN_PLAYER) && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
});

//Order the truck to build some defenses.
function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["AASite-QuadBof", "CO-WallTower-HvCan", "CO-Tower-RotMG", "CO-Tower-HvFlame"];
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("uplinkPos"));
}

//Attacks every 3 minutes until HQ is destroyed.
function wave2()
{
	const CONDITION = ((camAllowInsaneSpawns()) ? CAM_REINFORCE_CONDITION_BASES : "COCommandCenter");
	const list = [cTempl.colhvat, cTempl.colhvat];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), CONDITION, ext);
}

function wave3()
{
	const CONDITION = ((camAllowInsaneSpawns()) ? CAM_REINFORCE_CONDITION_BASES : "COCommandCenter");
	const list = [cTempl.commorv, cTempl.commorv];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), CONDITION, ext);
}

function vtolAttack()
{
	const CONDITION = ((camAllowInsaneSpawns()) ? CAM_REINFORCE_CONDITION_BASES : "COCommandCenter");
	if (camClassicMode())
	{
		const list = [cTempl.colatv, cTempl.colatv];
		const ext = {limit: [5, 5], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(2)), CONDITION, ext);
	}
	else
	{
		const list = [cTempl.commorvt, cTempl.commorvt];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), CONDITION, ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function insaneReinforcementSpawn()
{
	const DISTANCE_FROM_POS = 25;
	const units = [cTempl.comltath, cTempl.cohhvch, cTempl.comagh];
	const limits = {minimum: 5, maxRandom: 3};
	const location = camGenerateRandomMapEdgeCoordinate(getObject("startPosition"), CAM_GENERIC_WATER_STAT, DISTANCE_FROM_POS);
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_BASES, location, units, limits.minimum, limits.maxRandom);
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 25;
	const units = (!camClassicMode()) ? [cTempl.cocybsn, cTempl.cocybtk, cTempl.comltath, cTempl.cohhvch] : [cTempl.comltath, cTempl.cohhvch, cTempl.comagh];
	const limits = {minimum: 6, maxRandom: 4};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_BASES, location, units, limits.minimum, limits.maxRandom);
}

function insaneSetupSpawns()
{
	setTimer("insaneTransporterAttack", camMinutesToMilliseconds(2.5));
	setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(4));
}

//The project captured the uplink.
function captureUplink()
{
	playSound(cam_sounds.objectiveCaptured);
	hackRemoveMessage("C2D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
}

//Extra win condition callback.
function checkNASDACentral()
{
	if (getObject("uplink") === null)
	{
		return false; //It was destroyed
	}

	if (camCountStructuresInArea("uplinkClearArea", CAM_THE_COLLECTIVE) === 0)
	{
		camCallOnce("captureUplink");
		return true;
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Secure the Uplink from The Collective"));

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.beta8.pre, {
		area: "RTLZ",
		message: "C2D_LZ",
		reinforcements: camMinutesToSeconds(5),
		callback: "checkNASDACentral",
		annihilate: true,
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

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COCommandCenter": { tech: "R-Struc-Research-Upgrade04" },
			"COResearchLab": { tech: "R-Struc-VTOLPad-Upgrade01" },
			"COCommandRelay": { tech: "R-Wpn-Bomb02" },
			"COHeavyFactory": { tech: "R-Wpn-Howitzer-Accuracy01" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

		camUpgradeOnMapTemplates(cTempl.npcybf, cTempl.cocybth, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybc, cTempl.cocybsn, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybr, cTempl.cocybtk, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybm, cTempl.cocybag, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COCommandCenter": { tech: "R-Struc-VTOLPad-Upgrade01" },
			"COResearchLab": { tech: "R-Struc-Research-Upgrade04" },
			"COCommandRelay": { tech: "R-Wpn-Bomb02" },
			"COHeavyFactory": { tech: "R-Wpn-Howitzer-Accuracy01" },
			"COHowitzerEmplacement": { tech: "R-Wpn-Howitzer-Damage02" },
		});
	}

	setAlliance(CAM_HUMAN_PLAYER, MIS_UPLINK_PLAYER, true);
	setAlliance(CAM_THE_COLLECTIVE, MIS_UPLINK_PLAYER, true);

	camSetEnemyBases({
		"COSouthEastBase": {
			cleanup: "baseCleanup",
			detectMsg: "C2D_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COHeavyFactory": {
			assembly: "COHeavyFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 54 : 60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.cohhpv, cTempl.comhltat, cTempl.cohct]
		},
		"COSouthCyborgFactory": {
			assembly: "COSouthCyborgFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 36 : 40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cocybsn, cTempl.cocybth, cTempl.cocybtk, cTempl.cocybag] : [cTempl.npcybc, cTempl.npcybf, cTempl.npcybr, cTempl.cocybag]
		},
	});

	camManageTrucks(CAM_THE_COLLECTIVE);
	hackAddMessage("C2D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	camEnableFactory("COHeavyFactory");
	camEnableFactory("COSouthCyborgFactory");

	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(4)));
	if (camAllowInsaneSpawns())
	{
		queue("insaneSetupSpawns", camMinutesToMilliseconds(4));
	}

	truckDefense();
}
