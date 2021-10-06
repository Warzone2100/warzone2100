
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEW_PARADIGM_RES = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-Engineering01", "R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Defense-WallUpgrade03","R-Struc-Materials03", "R-Vehicle-Engine03",
	"R-Struc-RprFac-Upgrade03", "R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF03",
	"R-Vehicle-Metals03", "R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-RocketSlow-Damage03", "R-Wpn-Mortar-ROF01", "R-Cyborg-Metals03",
	"R-Wpn-Mortar-Acc01", "R-Wpn-RocketSlow-Accuracy01", "R-Wpn-Cannon-Accuracy01",
];
const SCAVENGER_RES = [
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Wpn-Rocket-Damage03",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-ROF03", "R-Vehicle-Metals02",
	"R-Defense-WallUpgrade03", "R-Struc-Materials03", "R-Wpn-Cannon-Accuracy01",
	"R-Wpn-Mortar-Acc01",
];
var artiGroup; //Droids that take the artifact
var enemyHasArtifact; //Do they have the artifact
var enemyStoleArtifact; //Reached the LZ with the artifact
var droidWithArtiID; //The droid ID that was closest to the artifact to take it
var artiMovePos; //where artiGroup members are moving to


//These enable scav factories when close enough
camAreaEvent("northScavFactoryTrigger", function(droid)
{
	camEnableFactory("scavNorthEastFactory");
});

camAreaEvent("southScavFactoryTrigger", function(droid)
{
	camEnableFactory("scavSouthEastFactory");
});

camAreaEvent("middleScavFactoryTrigger", function(droid)
{
	camEnableFactory("scavMiddleFactory");
});

//If a group member of artiGroup gets to the waypoint, then go to the
//New Paradigm landing zone.
camAreaEvent("NPWayPointTrigger", function(droid)
{
	artiMovePos = "NPTransportPos";
});

//Land New Paradigm transport if the New Paradigm have the artifact.
camAreaEvent("NPTransportTrigger", function(droid)
{
	if (enemyHasArtifact && droid.group === artiGroup)
	{
		var list = [cTempl.npmrl, cTempl.npmrl];
		camSendReinforcement(NEW_PARADIGM, camMakePos("NPTransportPos"), list, CAM_REINFORCE_TRANSPORT, {
			entry: { x: 39, y: 2 },
			exit: { x: 32, y: 60 }
		});
		playSound("pcv632.ogg"); //enemy transport escaping warning sound
	}
	else
	{
		resetLabel("NPTransportTrigger", NEW_PARADIGM);
	}
});

//Only called once when the New Paradigm takes the artifact for the first time.
function artifactVideoSetup()
{
	camPlayVideos({video: "SB1_7_MSG3", type: MISS_MSG});
	camCallOnce("removeCanyonBlip");
	artiMovePos = "NPWayPoint";
}

//Remove nearby droids. Make sure the player loses if the NP still has the artifact
//by the time it lands.
function eventTransporterLanded(transport)
{
	if (transport.player === NEW_PARADIGM && enemyHasArtifact)
	{
		enemyStoleArtifact = true;
		var crew = enumRange(transport.x, transport.y, 6, NEW_PARADIGM, false).filter(function(obj) {
			return obj.type === DROID && obj.group === artiGroup;
		});
		for (var i = 0, l = crew.length; i < l; ++i)
		{
			camSafeRemoveObject(crew[i], false);
		}
	}
}

//Check if the artifact group members are still alive and drop the artifact if needed.
function eventGroupLoss(obj, group, newsize)
{
	if (group === artiGroup && enemyHasArtifact && !enemyStoleArtifact)
	{
		if (obj.id === droidWithArtiID)
		{
			var acrate = addFeature("Crate", obj.x, obj.y);
			addLabel(acrate, "newArtiLabel");

			camSetArtifacts({
				"newArtiLabel": { tech: ["R-Wpn-Cannon3Mk1", "R-Wpn-RocketSlow-Damage03", "R-Wpn-Rocket-Damage03"] }
			});

			droidWithArtiID = undefined;
			enemyHasArtifact = false;
			hackRemoveMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER);
		}
	}
}

function enemyCanTakeArtifact(label)
{
	return label.indexOf("newArtiLabel") !== -1 || label.indexOf("artifactLocation") !== -1;
}

//Moves some New Paradigm forces to the artifact
function getArtifact()
{
	if (groupSize(artiGroup) === 0)
	{
		removeTimer("getArtifact");
		return;
	}

	const GRAB_RADIUS = 2;
	var artifact = camGetArtifacts().filter(function(label) {
		return enemyCanTakeArtifact(label) && getObject(label) !== null;
	});
	var artiLoc = artiMovePos;

	if (!enemyHasArtifact && !enemyStoleArtifact && artifact.length > 0)
	{
		//Go to the artifact instead.
		var realCrate = artifact[0];
		artiLoc = camMakePos(realCrate);
		if (!camDef(artiLoc))
		{
			return; //player must have snatched it
		}

		//Find the one closest to the artifact so that one can "hold" it
		var artiMembers = enumGroup(artiGroup);
		var idx = 0;
		var dist = Infinity;

		for (var i = 0, l = artiMembers.length; i < l; ++i)
		{
			var drDist = camDist(artiMembers[i], artiLoc);
			if (drDist < dist)
			{
				idx = i;
				dist = drDist;
			}
		}

		//Now take it if close enough
		if (camDist(artiMembers[idx], artiLoc) < GRAB_RADIUS)
		{
			camCallOnce("artifactVideoSetup");
			hackAddMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER, false); //NPLZ blip
			droidWithArtiID = artiMembers[idx].id;
			enemyHasArtifact = true;
			camSafeRemoveObject(realCrate, false);
		}
	}

	if (camDef(artiLoc))
	{
		camManageGroup(artiGroup, CAM_ORDER_DEFEND, {
			pos: artiLoc,
			radius: 0,
			regroup: false
		});
	}
}

//New Paradigm truck builds six lancer hardpoints around LZ
function buildLancers()
{
	for (var i = 1; i <= 6; ++i)
	{
		camQueueBuilding(NEW_PARADIGM, "WallTower06", "hardPoint" + i);
	}
}

//Must destroy all of the New Paradigm droids and make sure the artifact is safe.
function extraVictory()
{
	var npTransportFound = false;
	enumDroid(NEW_PARADIGM).forEach(function(dr) {
		if (camIsTransporter(dr))
		{
			npTransportFound = true;
		}
	});

	//fail if they stole it and the transporter is not on map anymore
	if (enemyStoleArtifact && !npTransportFound)
	{
		return false;
	}

	if (!enumDroid(NEW_PARADIGM).length)
	{
		return true;
	}
}

function removeCanyonBlip()
{
	hackRemoveMessage("C1-7_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
}

function eventPickup(feature, droid)
{
	if (feature.stattype === ARTIFACT)
	{
		if (droid.player === CAM_HUMAN_PLAYER)
		{
			if (enemyHasArtifact)
			{
				hackRemoveMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER);
			}
			enemyHasArtifact = false;
			camCallOnce("removeCanyonBlip");
		}
	}
}

function startArtifactCollection()
{
	setTimer("getArtifact", camSecondsToMilliseconds(0.2));
}

//Mission setup stuff
function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Destroy all New Paradigm units"));

	enemyHasArtifact = false;
	enemyStoleArtifact = false;
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_DS", {
		area: "RTLZ",
		message: "C1-7_LZ",
		reinforcements: camMinutesToSeconds(1),
		callback: "extraVictory",
		retlz: true,
	});

	//Make sure the New Paradigm and Scavs are allies
	setAlliance(NEW_PARADIGM, SCAV_7, true);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"artifactLocation": { tech: ["R-Wpn-Cannon3Mk1", "R-Wpn-RocketSlow-Damage03"] },
	});

	camCompleteRequiredResearch(NEW_PARADIGM_RES, NEW_PARADIGM);
	camCompleteRequiredResearch(SCAVENGER_RES, SCAV_7);

	camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.trike, cTempl.trikeheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggyheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeepheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.rbjeep, cTempl.rbjeep8, SCAV_7);

	// New MRA Mantis Tracks units on the hill
	addDroid(NEW_PARADIGM, 29, 16, "MRA Mantis Tracks", "Body12SUP", "tracked01", "", "", "Rocket-MRL");
	addDroid(NEW_PARADIGM, 29, 17, "MRA Mantis Tracks", "Body12SUP", "tracked01", "", "", "Rocket-MRL");
	addDroid(NEW_PARADIGM, 29, 18, "MRA Mantis Tracks", "Body12SUP", "tracked01", "", "", "Rocket-MRL");

	camSetEnemyBases({
		"ScavMiddleGroup": {
			cleanup: "scavMiddle",
			detectMsg: "C1-7_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"ScavSouthEastGroup": {
			cleanup: "scavSouthEast",
			detectMsg: "C1-7_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"ScavNorthEastGroup": {
			cleanup: "scavNorth",
			detectMsg: "C1-7_BASE3",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
	});

	camSetFactories({
		"scavMiddleFactory": {
			assembly: "middleAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(10)),
			data: {
				regroup: true,
				count: -1,
			},
			templates: [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ]
		},
		"scavSouthEastFactory": {
			assembly: "southAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(10)),
			data: {
				regroup: true,
				count: -1,
			},
			templates: [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ]
		},
		"scavNorthEastFactory": {
			assembly: "northAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(10)),
			rdata: {
				regroup: true,
				count: -1,
			},
			templates: [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ]
		},
	});

	artiGroup = camMakeGroup(enumArea("NPArtiGroup", NEW_PARADIGM, false));
	droidWithArtiID = 0;
	camManageTrucks(NEW_PARADIGM);
	buildLancers();

	hackAddMessage("C1-7_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false); //Canyon
	queue("startArtifactCollection", camChangeOnDiff(camMinutesToMilliseconds(1.5)));
}
