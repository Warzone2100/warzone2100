
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const SCAVENGER_RES = [
	"R-Wpn-Flamer-Damage02", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage02", "R-Wpn-MG-ROF01", "R-Wpn-Mortar-Damage02",
	"R-Wpn-Mortar-ROF01", "R-Wpn-Rocket-ROF03",
];

function exposeNorthBase()
{
	camDetectEnemyBase("NorthGroup"); // no problem if already detected
	camPlayVideos({video: "SB1_2_MSG2", type: MISS_MSG});
}

function camArtifactPickup_ScavLab()
{
	camCallOnce("exposeNorthBase");
	camSetFactoryData("WestFactory", {
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
		throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty === EASY || difficulty === MEDIUM) ? 13 : 10)),
		templates: [ cTempl.trikeheavy, cTempl.blokeheavy, cTempl.buggyheavy, cTempl.bjeepheavy ]
	});
	camEnableFactory("WestFactory");
}

function camEnemyBaseDetected_NorthGroup()
{
	camCallOnce("exposeNorthBase");
}

camAreaEvent("NorthBaseTrigger", function(droid)
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
		reinforcements: 60,
		retlz: true
	});

	var startpos = getObject("StartPosition");
	var lz = getObject("LandingZone");
	var tent = getObject("TransporterEntry");
	var text = getObject("TransporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camCompleteRequiredResearch(SCAVENGER_RES, SCAV_7);

	camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.trike, cTempl.triketwin, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggytwin, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeeptwin, SCAV_7);

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
			cleanup: "ScavLabCleanup",
			detectMsg: "C1-2_OBJ1",
		},
	});

	camDetectEnemyBase("ScavLabGroup");

	camSetArtifacts({
		"ScavLab": { tech: ["R-Wpn-Mortar01Lt", "R-Wpn-Flamer-Damage02"] },
		"NorthFactory": { tech: ["R-Vehicle-Prop-Halftracks", "R-Wpn-Cannon1Mk1"] },
	});

	camSetFactories({
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty === EASY || difficulty === MEDIUM) ? 20 : 15)),
			group: camMakeGroup("NorthTankGroup"),
			templates: [ cTempl.trikeheavy, cTempl.blokeheavy, cTempl.buggyheavy, cTempl.bjeepheavy ]
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty === EASY || difficulty === MEDIUM) ? 13 : 10)),
			templates: [ cTempl.trikeheavy, cTempl.blokeheavy, cTempl.buggyheavy, cTempl.bjeepheavy ]
		},
	});

	queue("enableWestFactory", camSecondsToMilliseconds(30));
}
