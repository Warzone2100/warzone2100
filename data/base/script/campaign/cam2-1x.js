/* 
SUB_2_1 Script
Authors: Cristian Odorico (Alpha93) / KJeff01
 */
//libraries initialization
include ("script/campaign/libcampaign.js");
include ("script/campaign/templates.js");

//constants initialization
const downedTransportTeam = 1;
const CO = 2;
const collectiveRes = [
	"R-Defense-WallUpgrade03", "R-Struc-Materials03", "R-Vehicle-Engine04",
	"R-Vehicle-Metals03", "R-Cyborg-Metals03", "R-Vehicle-Armor-Heat01",
	"R-Cyborg-Armor-Heat01", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04",
	"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage04", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage03", "R-Wpn-RocketSlow-ROF03"
];

//function that applies damage to units in the downed transport transport team
function preDamageUnits()
{
	var transport = enumStruct(downedTransportTeam);
	setHealth(transport[0], 40);
	// fill the array with the objects defining the allied units in the crash site area
	var downedTransportUnits = enumDroid(downedTransportTeam);
	for (var j = 0; j < downedTransportUnits.length; j++)
	{
		setHealth(downedTransportUnits[j], 40 + camRand(20));
	}
}

function proceedToNextLevel()
{
	//turn time into power
	extraPowerTime(getMissionTime(), CAM_HUMAN_PLAYER);
	//load next level
	camNextLevel("CAM_2B");
}
                          
//trigger event when droid reaches the downed transport
camAreaEvent("crashSite", function(droid)
{
	//initialize function specific variables
	const successSound = "pcv615.ogg";
	const failureSound = "pcv622.ogg";
	//count structures in the crash area
	var transport = enumStruct(downedTransportTeam).length;
	var customVictoryFlag = 0;
	var remainingTime = 0;

	//set downed transport team colour to be Project Green
	changePlayerColour(downedTransportTeam, 0);

	//remove blip
	hackRemoveMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER, true);
	//victory condition: transport must be alive
	if (transport === 1)
	{
		playSound(successSound);
		customVictoryFlag = 1;
	}
	else
	{
		//if transport died, the game is over
		playSound(failureSound);
		gameOverMessage(false);
	}             
	//if transport is alive, transfer units, turn time into power and load next level
	if (customVictoryFlag === 1)
	{
		//get a list of droids in the downed transport team array
		var downedTransportUnits = enumDroid(downedTransportTeam);
		for(var i = 0; i < downedTransportUnits.length; i++)
		{
			//transfer the units
			donateObject(downedTransportUnits[i], CAM_HUMAN_PLAYER);
		}
		queue("proceedToNextLevel", 2000);
	}
});

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
	var downedTransportUnits = enumDroid(downedTransportTeam);
	for(var i = 0; i < downedTransportUnits.length; i++)
	{
		if(camDef(downedTransportUnits[i]))
		{
			var temp = downedTransportUnits[i];
			if(camDef(temp.weapons[0]))
			{
				addDroid(downedTransportTeam, temp.x, temp.y, "Team Alpha unit",
					temp.body, temp.propulsion, "", "", temp.weapons[0].name);
			}
			else
			{
				addDroid(downedTransportTeam, temp.x, temp.y, "Team Alpha unit",
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

function eventStartLevel()
{
	//variables initialization for LZ setup
	var subLandingZone = getObject("landingZone");
	//set landing zone
	setNoGoArea(subLandingZone.x, subLandingZone.y, subLandingZone.x2, subLandingZone.y2);
	//set alliance between player and AI downed transport team
	setAlliance(CAM_HUMAN_PLAYER, downedTransportTeam, true);
	//disable reinforcements
	setReinforcementTime(-1);
	//centre view on starting position
	var startpos = getObject("startingPosition");
	centreView(startpos.x, startpos.y);
	//Setup transporter entry/exit points
	var tent = getObject("transporterEntry");
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	var text = getObject("transporterExit");
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);
	//add crash site blip
	hackAddMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER, true);

	camEnableRes(CO, collectiveRes);
	/*
	camSetEnemyBases({
		"COHardpointBase": {
			cleanup: "hardpointBaseCleanup",
			detectMsg: "C21_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
		"COBombardBase": {
			cleanup: "bombardBaseCleanup",
			detectMsg: "C21_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
		"COBunkerBase": {
			cleanup: "bunkerBaseCleanup",
			detectMsg: "C21_BASE3",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
	});
	*/

	setupCyborgGroups();
	queue("updateTransportUnits", 20000);
};

