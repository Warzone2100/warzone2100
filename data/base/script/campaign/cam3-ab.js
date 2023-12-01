include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_nexusRes = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade09", "R-Struc-Materials09",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage09", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine08", "R-Vehicle-Metals08", "R-Vehicle-Armor-Heat05",
	"R-Cyborg-Metals08", "R-Cyborg-Armor-Heat05", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01", "R-Wpn-Missile-Accuracy01",
	"R-Wpn-Rail-Damage01", "R-Wpn-Rail-ROF01", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03",
];
var hackFailChance; //chance the Nexus Intruder Program will fail
var winFlag;

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER)
	{
		if (isVTOL(droid))
		{
			camSafeRemoveObject(droid, false);
		}
	}

	resetLabel("vtolRemoveZone", CAM_NEXUS);
});

function sendEdgeMapDroids()
{
	let unitCount = 16 + camRand(5); // 16 - 20.
	if (difficulty === INSANE)
	{
		unitCount = 14 + camRand(3); // 14 - 16.
	}
	const edge = ["SWPhantomFactory", "NWPhantomFactory"];
	let list = [
		cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas,
		cTempl.nxlflash, cTempl.nxmrailh, cTempl.nxmlinkh,
		cTempl.nxmscouh, cTempl.nxmsamh,
	];
	if (difficulty >= HARD)
	{
		list = list.concat(cTempl.nxmangel);
	}
	let droids = [];

	for (let i = 0; i < unitCount; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	droids = droids.concat(cTempl.nxmsens);

	camSendReinforcement(CAM_NEXUS, camMakePos(edge[camRand(edge.length)]), droids,
		CAM_REINFORCE_GROUND, {
			data: {regroup: false, count: -1}
		}
	);
}

function wave2()
{
	const list = [cTempl.nxlscouv, cTempl.nxlscouv];
	const ext = {
		limit: [3, 3], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_NEXUS, (difficulty === INSANE) ? undefined : "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(3.5)), undefined, ext);
}

function wave3()
{
	const list = [cTempl.nxlneedv, cTempl.nxlneedv];
	const ext = {
		limit: [3, 3], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_NEXUS, (difficulty === INSANE) ? undefined : "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(3.5)), undefined, ext);
}

//Setup Nexus VTOL hit and runners. NOTE: These do not go away in this mission.
function vtolAttack()
{
	const list = [cTempl.nxmtherv, cTempl.nxmheapv];
	const ext = {
		limit: [3, 3], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_NEXUS, (difficulty === INSANE) ? undefined : "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(3.5)), undefined, ext);
	queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
	queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
}

// Order any absorbed trucks to start building defenses near themselves.
function truckDefense()
{
	const droids = enumDroid(CAM_NEXUS, DROID_CONSTRUCT);
	const defenses = [
		"Sys-NEXUSLinkTOW", "P0-AASite-SAM2", "Emplacement-PrisLas",
		"NX-Tower-ATMiss", "Sys-NX-CBTower",
	];

	for (let i = 0, len = droids.length; i < len; ++i)
	{
		const truck = droids[i];
		if (truck.order !== DORDER_BUILD)
		{
			const _DEFENSE = defenses[camRand(defenses.length)];
			const loc = pickStructLocation(truck, _DEFENSE, truck.x, truck.y);
			enableStructure(_DEFENSE, CAM_NEXUS);
			if (camDef(loc))
			{
				orderDroidBuild(truck, DORDER_BUILD, _DEFENSE, truck.x, truck.y);
			}
		}
	}
}

function hackManufacture(structure, template)
{
	makeComponentAvailable(template.body, structure.player);
	makeComponentAvailable(template.prop, structure.player);
	makeComponentAvailable(template.weap, structure.player);
	return buildDroid(structure, "Nexus unit", template.body, template.prop, "", "", template.weap);
}

function nexusManufacture()
{
	if (countDroid(DROID_ANY, CAM_NEXUS) > 100)
	{
		return;
	}
	const factoryType = [
		{structure: FACTORY, temps: [cTempl.nxmrailh, cTempl.nxmlinkh, cTempl.nxmscouh, cTempl.nxlflash,]},
		{structure: CYBORG_FACTORY, temps: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas,]},
		{structure: VTOL_FACTORY, temps: [cTempl.nxlscouv, cTempl.nxmtherv, cTempl.nxmheapv,]},
	];

	for (let i = 0; i < factoryType.length; ++i)
	{
		const factories = enumStruct(CAM_NEXUS, factoryType[i].structure);
		const templs = factoryType[i].temps;

		for (let j = 0, len = factories.length; j < len; ++j)
		{
			const fac = factories[j];
			if (fac.status !== BUILT || !structureIdle(fac))
			{
				return;
			}
			hackManufacture(fac, templs[camRand(templs.length)]);
		}
	}

	queue("manualGrouping", camSecondsToMilliseconds(1.5));
}

function manualGrouping()
{
	const vtols = enumDroid(CAM_NEXUS).filter((obj) => (obj.group === null && isVTOL(obj)));
	const nonVtols = enumDroid(CAM_NEXUS).filter((obj) => (obj.group === null && !isVTOL(obj)));
	if (vtols.length)
	{
		camManageGroup(camMakeGroup(vtols), CAM_ORDER_ATTACK, { regroup: false, count: -1 });
	}
	if (nonVtols.length)
	{
		camManageGroup(camMakeGroup(nonVtols), CAM_ORDER_ATTACK, { regroup: false, count: -1 });
	}
}

function eventObjectTransfer(obj, from)
{
	if (obj.player === CAM_NEXUS && from === CAM_HUMAN_PLAYER)
	{
		if (obj.type === STRUCTURE)
		{
			if (obj.stattype === FACTORY || obj.stattype === CYBORG_FACTORY || obj.stattype === VTOL_FACTORY)
			{
				queue("nexusManufacture", camSecondsToMilliseconds(0.1)); //build immediately if possible.
			}
		}
	}
}

function powerTransfer()
{
	setPower(playerPower(CAM_HUMAN_PLAYER) + 5000);
	playSound(cam_sounds.powerTransferred);
}

function eventResearched(research, structure, player)
{
	if (research.name === cam_resistance_circuits.first)
	{
		hackFailChance = 60;
	}
	else if (research.name === cam_resistance_circuits.second)
	{
		hackFailChance = 75;
	}
	else if (research.name === cam_resistance_circuits.third)
	{
		hackFailChance = 90;
	}
	else if (research.name === cam_resistance_circuits.fourth)
	{
		winFlag = true;
		hackFailChance = 100;
		camSetNexusState(false);
	}
}

function hackPlayer()
{
	if (!camGetNexusState())
	{
		removeTimer("hackPlayer");
		return;
	}
	if (camRand(100) < hackFailChance)
	{
		return;
	}

	camHackIntoPlayer(CAM_HUMAN_PLAYER, CAM_NEXUS);
}

function synapticsSound()
{
	playSound(cam_sounds.nexus.synapticLinksActivated);
	camHackIntoPlayer(CAM_HUMAN_PLAYER, CAM_NEXUS);
}

//winFlag is set in eventResearched.
function resistanceResearched()
{
	if (winFlag)
	{
		return true;
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Research resistance circuits and survive the assault from Nexus"));

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM3C", {
		callback: "resistanceResearched"
	});

	camSetNexusState(true);
	camPlayVideos([{video: "MB3_AB_MSG", type: CAMP_MSG}, {video: "MB3_AB_MSG2", type: CAMP_MSG}, {video: "MB3_AB_MSG3", type: MISS_MSG}]);

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));

	const enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, CAM_NEXUS);

	camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);

	enableResearch(cam_resistance_circuits.first, CAM_HUMAN_PLAYER);
	winFlag = false;
	hackFailChance = (difficulty <= EASY) ? 45 : 33;

	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(2)));

	queue("powerTransfer", camSecondsToMilliseconds(0.8));
	queue("synapticsSound", camSecondsToMilliseconds(2.5));
	queue("sendEdgeMapDroids", camSecondsToMilliseconds(15));

	setTimer("truckDefense", camSecondsToMilliseconds(2));
	setTimer("hackPlayer", camChangeOnDiff(camSecondsToMilliseconds(8)));
	setTimer("nexusManufacture", camSecondsToMilliseconds(10));
	setTimer("sendEdgeMapDroids", camChangeOnDiff(camMinutesToMilliseconds(4)));
}
