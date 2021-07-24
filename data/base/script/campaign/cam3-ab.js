include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
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
var edgeMapCounter; //how many Nexus reinforcement runs have happened.
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

	resetLabel("vtolRemoveZone", NEXUS);
});

function sendEdgeMapDroids()
{
	var unitCount = 16 + camRand(5); // 16 - 20.
	if (difficulty === INSANE)
	{
		unitCount = 14 + camRand(3); // 14 - 16.
	}
	const EDGE = ["SWPhantomFactory", "NWPhantomFactory"];
	var list = [
		cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas,
		cTempl.nxlflash, cTempl.nxmrailh, cTempl.nxmlinkh,
		cTempl.nxmscouh, cTempl.nxmsamh,
	];
	if (difficulty >= HARD)
	{
		list = list.concat(cTempl.nxmangel);
	}
	var droids = [];

	if (!camDef(edgeMapCounter))
	{
		edgeMapCounter = 0;
	}

	for (var i = 0; i < unitCount; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	droids = droids.concat(cTempl.nxmsens);

	camSendReinforcement(NEXUS, camMakePos(EDGE[camRand(EDGE.length)]), droids,
		CAM_REINFORCE_GROUND, {
			data: {regroup: false, count: -1}
		}
	);

	edgeMapCounter += 1;
}

//Setup Nexus VTOL hit and runners. NOTE: These do not go away in this mission.
function vtolAttack()
{
	var list = [cTempl.nxlscouv, cTempl.nxmtherv, cTempl.nxlscouv, cTempl.nxmheapv];
	var ext = {
		limit: [2, 4, 2, 4],
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(NEXUS, (difficulty === INSANE) ? undefined : "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(2)), undefined, ext);
}

// Order any absorbed trucks to start building defenses near themselves.
function truckDefense()
{
	var droids = enumDroid(NEXUS, DROID_CONSTRUCT);
	var defenses = [
		"Sys-NEXUSLinkTOW", "P0-AASite-SAM2", "Emplacement-PrisLas",
		"NX-Tower-ATMiss", "Sys-NX-CBTower",
	];

	for (var i = 0, len = droids.length; i < len; ++i)
	{
		var truck = droids[i];
		if (truck.order !== DORDER_BUILD)
		{
			var defense = defenses[camRand(defenses.length)];
			var loc = pickStructLocation(truck, defense, truck.x, truck.y);
			enableStructure(defense, NEXUS);
			if (camDef(loc))
			{
				orderDroidBuild(truck, DORDER_BUILD, defense, truck.x, truck.y);
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
	if (countDroid(DROID_ANY, NEXUS) > 100)
	{
		return;
	}
	var factoryType = [
		{structure: FACTORY, temps: [cTempl.nxmrailh, cTempl.nxmlinkh, cTempl.nxmscouh, cTempl.nxlflash,]},
		{structure: CYBORG_FACTORY, temps: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas,]},
		{structure: VTOL_FACTORY, temps: [cTempl.nxlscouv, cTempl.nxmtherv, cTempl.nxmheapv,]},
	];

	for (var i = 0; i < factoryType.length; ++i)
	{
		var factories = enumStruct(NEXUS, factoryType[i].structure);
		var templs = factoryType[i].temps;

		for (var j = 0, len = factories.length; j < len; ++j)
		{
			var fac = factories[j];
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
	var vtols = enumDroid(NEXUS).filter(function(obj) {
		return obj.group === null && isVTOL(obj);
	});
	var nonVtols = enumDroid(NEXUS).filter(function(obj) {
		return obj.group === null && !isVTOL(obj);
	});
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
	if (obj.player === NEXUS && from === CAM_HUMAN_PLAYER)
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
	playSound("power-transferred.ogg");
}

function eventResearched(research, structure, player)
{
	if (research.name === "R-Sys-Resistance-Upgrade01")
	{
		hackFailChance = 55;
	}
	else if (research.name === "R-Sys-Resistance-Upgrade02")
	{
		hackFailChance = 80;
	}
	else if (research.name === "R-Sys-Resistance-Upgrade03")
	{
		winFlag = true;
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

	camHackIntoPlayer(CAM_HUMAN_PLAYER, NEXUS);
}

function synapticsSound()
{
	playSound(SYNAPTICS_ACTIVATED);
	camHackIntoPlayer(CAM_HUMAN_PLAYER, NEXUS);
}

//winFlag is set in eventResearched.
function resistanceResearched()
{
	const MIN_EDGE_COUNT = 15;
	if (winFlag && edgeMapCounter >= MIN_EDGE_COUNT)
	{
		return true;
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Research resistance circuits and survive the assault from Nexus"));

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM3C", {
		callback: "resistanceResearched"
	});

	camSetNexusState(true);
	camPlayVideos(["MB3_AB_MSG", "MB3_AB_MSG2", "MB3_AB_MSG3"]);

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));

	var enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, NEXUS);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);

	enableResearch("R-Sys-Resistance-Upgrade01", CAM_HUMAN_PLAYER);
	winFlag = false;
	hackFailChance = 30;

	vtolAttack();

	queue("powerTransfer", camSecondsToMilliseconds(0.8));
	queue("synapticsSound", camSecondsToMilliseconds(2.5));
	queue("sendEdgeMapDroids", camSecondsToMilliseconds(15));

	setTimer("truckDefense", camSecondsToMilliseconds(2));
	setTimer("hackPlayer", camSecondsToMilliseconds((difficulty <= HARD) ? 8 : 5));
	setTimer("nexusManufacture", camSecondsToMilliseconds(10));
	setTimer("sendEdgeMapDroids", camChangeOnDiff(camMinutesToMilliseconds(3)));
}
