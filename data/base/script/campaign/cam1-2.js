
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var northSeen = false;

function exposeNorthBase() {
	if (northSeen)
		return;
	northSeen = true;
	camDetectEnemyBase("NorthGroup"); // no problem if already detected
	hackAddMessage("SB1_2_MSG2", MISS_MSG, 0, true); // that's what it was for
}

function camArtifactPickup_ScavLab() {
	exposeNorthBase();
	with (camTemplates) camSetFactoryData("WestFactory", {
		assembly: "WestAssembly",
		order: CAM_ORDER_COMPROMISE,
		data: {
			pos: [
				camMakePos("WestAssembly"),
				camMakePos("GatesPos"),
				camMakePos("RTLZ"), // changes
			],
			radius: 8
		},
		groupSize: 5,
		maxSize: 9,
		throttle: 10000,
		templates: [ trike, bloke, buggy, bjeep ]
	});
	camEnableFactory("WestFactory");
}

function camEnemyBaseDetected_NorthGroup() {
	exposeNorthBase();
}

camAreaEvent("NorthBaseTrigger", 0, function(droid)
{
	// frankly, this one happens silently
	camEnableFactory("NorthFactory");
});

function enableWestFactory()
{
	camEnableFactory("WestFactory");
	camManageGroup(camMakeGroup("RaidGroup"), CAM_ORDER_ATTACK, {
		pos: camMakePos("RTLZ"),
		morale: 80,
		fallback: camMakePos("ScavLab")
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_3S", {
		area: "RTLZ",
		message: "C1-2_LZ",
		reinforcements: 60
	});
	var startpos = getObject("StartPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("LandingZone");
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);
	var tent = getObject("TransporterEntry");
	startTransporterEntry(62, 58, 0);
	var text = getObject("TransporterExit");
	setTransporterExit(58, 62, 0);
	setPower(400, 7);

	camSetEnemyBases({
		"NorthGroup": {
			cleanup: "NorthBase",
			detectMsg: "C1-2_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"WestGroup": {
			cleanup: "WestBase",
			detectMsg: "C1-2_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"ScavLabGroup": {
			detectMsg: "C1-2_OBJ1",
		},
	});

	hackAddMessage("SB1_2_MSG", MISS_MSG, 0, false);
	camDetectEnemyBase("ScavLabGroup");

	camSetArtifacts({
		"ScavLab": { tech: "R-Wpn-Mortar01Lt" },
		"NorthFactory": { tech: "R-Vehicle-Prop-Halftracks" },
	});

	with (camTemplates) camSetFactories({
		"NorthFactory": {
			assembly: "NorthAssembly",
			order: CAM_ORDER_COMPROMISE,
			data: {
				pos: [
					camMakePos("NorthAssembly"),
					camMakePos("ScavLabPos"),
					camMakePos("RTLZ"),
				],
				radius: 8
			},
			groupSize: 5,
			maxSize: 9,
			throttle: 15000,
			group: camMakeGroup("NorthTankGroup"),
			templates: [ trike, bloke, buggy, bjeep ]
		},
		"WestFactory": {
			assembly: "WestAssembly",
			order: CAM_ORDER_COMPROMISE,
			data: {
				pos: [
					camMakePos("WestAssembly"),
					camMakePos("GatesPos"),
					camMakePos("ScavLabPos"),
				],
				radius: 8
			},
			groupSize: 5,
			maxSize: 9,
			throttle: 10000,
			templates: [ trike, bloke, buggy, bjeep ]
		},
	});

	queue("enableWestFactory", 30000);
}
