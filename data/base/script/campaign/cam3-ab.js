include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals07",
	"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Accuracy03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage02",
	"R-Wpn-Energy-ROF02", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
];
var edgeMapCounter; //how many Nexus reinforcement runs have happened.
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
	const COUNT = 16 + camRand(5); // 16 - 20.
	const EDGE = ["SWPhantomFactory", "NWPhantomFactory"];
	var list = [
		cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas,
		cTempl.nxlflash, cTempl.nxmrailh, cTempl.nxmlinkh,
		cTempl.nxmscouh, cTempl.nxmsamh, cTempl.nxmstrike,
	];
	var droids = [];

	if (!camDef(edgeMapCounter))
	{
		edgeMapCounter = 0;
	}

	for (var i = 0; i < COUNT; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(NEXUS, camMakePos(EDGE[camRand(EDGE.length)]), droids,
		CAM_REINFORCE_GROUND, {
			data: {regroup: false, count: -1}
		}
	);

	edgeMapCounter += 1;
	queue("sendEdgeMapDroids", camChangeOnDiff(180000)); // 3 min.
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
	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(120000), undefined, ext); //2 min
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
		{structure: "A0LightFactory", temps: [cTempl.nxmrailh, cTempl.nxmlinkh, cTempl.nxmscouh, cTempl.nxlflash,]},
		{structure: "A0CyborgFactory", temps: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas,]},
		{structure: "A0VTolFactory1", temps: [cTempl.nxlscouv, cTempl.nxmtherv, cTempl.nxmheapv,]},
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

	queue("manualGrouping", 1500);
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
				queue("nexusManufacture", 100); //build immediately if possible.
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
		camSetNexusState(false);
	}
	if (research.name === "R-Sys-Resistance-Upgrade03")
	{
		winFlag = true;
	}
}

function hackPlayer()
{
	camHackIntoPlayer(CAM_HUMAN_PLAYER, NEXUS);
	if (camGetNexusState())
	{
		queue("hackPlayer", 5000);
	}
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

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM3C", {
		callback: "resistanceResearched"
	});

	camSetNexusState(true);
	camPlayVideos(["MB3_AB_MSG", "MB3_AB_MSG2", "MB3_AB_MSG3"]);

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(3600)); //1 hour.

	var enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, NEXUS);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);

	enableResearch("R-Sys-Resistance-Upgrade01", CAM_HUMAN_PLAYER);
	winFlag = false;

	vtolAttack();

	queue("powerTransfer", 800);
	queue("synapticsSound", 2500);
	queue("hackPlayer", 8000);
	queue("sendEdgeMapDroids", 15000); // 15 sec

	setTimer("truckDefense", 2000);
	setTimer("nexusManufacture", 10000);
}
