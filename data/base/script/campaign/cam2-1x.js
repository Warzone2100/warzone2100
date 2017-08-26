/*
SUB_2_1 Script
Authors: Cristian Odorico (Alpha93) / KJeff01
 */
include ("script/campaign/libcampaign.js");
include ("script/campaign/templates.js");

const TRANSPORT_TEAM = 1;
const COLLECTIVE_RES = [
	"R-Defense-WallUpgrade03", "R-Struc-Materials03", "R-Vehicle-Engine04",
	"R-Vehicle-Metals03", "R-Cyborg-Metals03", "R-Vehicle-Armor-Heat01",
	"R-Cyborg-Armor-Heat01", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04",
	"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage04", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage03", "R-Wpn-RocketSlow-ROF03"
];

//trigger event when droid reaches the downed transport.
camAreaEvent("crashSite", function(droid)
{
	//Unlikely to happen.
	if(enumDroid(TRANSPORT_TEAM).length === 0)
	{
		gameOverMessage(false);
		return;
	}

	const GOODSND = "pcv615.ogg";
	playSound(GOODSND);

	//set downed transport team colour to be Project Green.
	changePlayerColour(TRANSPORT_TEAM, 0);
	hackRemoveMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER, true);

	var downedTransportUnits = enumDroid(TRANSPORT_TEAM);
	for(var i = 0; i < downedTransportUnits.length; i++)
	{
		donateObject(downedTransportUnits[i], CAM_HUMAN_PLAYER);
	}

	//Give the donation enough time to transfer them to the player. Otherwise
	//the level will end too fast and will trigger asserts in the next level.
	queue("triggerWin", 2000);
});

//function that applies damage to units in the downed transport transport team.
function preDamageUnits()
{
	setHealth(getObject("transporter"), 40);
	// fill the array with the objects defining the allied units in the crash site area
	var downedTransportUnits = enumDroid(TRANSPORT_TEAM);
	for (var j = 0; j < downedTransportUnits.length; j++)
	{
		setHealth(downedTransportUnits[j], 40 + camRand(20));
	}
}

//victory callback will thus complete the level.
function triggerWin()
{
	victoryFlag = true;
}

function setupCyborgGroups()
{
	//create group of cyborgs and send them on war path
	camManageGroup(camMakeGroup("cyborgPositionNorth"), CAM_ORDER_ATTACK, {
		regroup: false
	});

	//East cyborg group patrols around the bombard pits
	camManageGroup(camMakeGroup("cyborgPositionEast"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos ("cybEastPatrol1"),
			camMakePos ("cybEastPatrol2"),
			camMakePos ("cybEastPatrol3"),
			camMakePos ("cybEastPatrol4"),
		],
		interval: 15000,
		regroup: true
	});
}

//Destroy all the droids from the downed team to give them a valid droid ID
//and recreate them.
function updateTransportUnits()
{
	var downedTransportUnits = enumDroid(TRANSPORT_TEAM);
	for(var i = 0; i < downedTransportUnits.length; i++)
	{
		if(camDef(downedTransportUnits[i]))
		{
			var temp = downedTransportUnits[i];
			if(camDef(temp.weapons[0]))
			{
				addDroid(TRANSPORT_TEAM, temp.x, temp.y, "Team Alpha unit",
					temp.body, temp.propulsion, "", "", temp.weapons[0].name);
			}
			else
			{
				addDroid(TRANSPORT_TEAM, temp.x, temp.y, "Team Alpha unit",
					temp.body, temp.propulsion, "", "", "Spade1Mk1");
			}
		}
	}

	//Remove the old droids.
	for(var i = 0; i < downedTransportUnits.length; i++)
	{
		if(camDef(downedTransportUnits[i]))
		{
			camSafeRemoveObject(downedTransportUnits[i], false);
		}
	}

	preDamageUnits();
}

//Checks if the downed tranport has been destroyed and issues a game lose. This
//likely will never happen as with the WZ Script version.
function checkCrashedTeam()
{
	if(getObject("transporter") === null)
	{
		const BADSND = "pcv622.ogg";
		playSound(BADSND);
		return false;
	}

	if(camDef(victoryFlag) && (victoryFlag === true))
	{
		return true;
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_2B", {
		area: "RTLZ",
		message: "C21_LZ",
		reinforcements: -1,
		callback: "checkCrashedTeam"
	});

	var subLandingZone = getObject("landingZone");
	var startpos = getObject("startingPosition");
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(subLandingZone.x, subLandingZone.y, subLandingZone.x2, subLandingZone.y2);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Add crash site blip and from an alliance with the crashed team.
	hackAddMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER, true);
	setAlliance(CAM_HUMAN_PLAYER, TRANSPORT_TEAM, true);

	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	camSetEnemyBases({
		"COHardpointBase": {
			cleanup: "hardpointBaseCleanup",
			detectMsg: "C21_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COBombardBase": {
			cleanup: "bombardBaseCleanup",
			detectMsg: "C21_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COBunkerBase": {
			cleanup: "bunkerBaseCleanup",
			detectMsg: "C21_BASE3",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	victoryFlag = false;
	queue("setupCyborgGroups", 5000);
	queue("updateTransportUnits", 20000);
}
