include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_scavengerRes = [
	"R-Wpn-Flamer-Damage02", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage02", "R-Wpn-MG-ROF01", "R-Wpn-Mortar-Damage02",
	"R-Wpn-Mortar-ROF01", "R-Wpn-Rocket-ROF03",
	"R-Defense-WallUpgrade01", "R-Struc-Materials01",
];

// CLASSIC: No research.

function exposeNorthBase()
{
	camDetectEnemyBase("NorthGroup"); // no problem if already detected
	camPlayVideos({video: "SB1_2_MSG2", type: MISS_MSG});
	if (camAllowInsaneSpawns())
	{
		insaneReinforcementSpawn();
		setTimer("insaneReinforcementSpawn", camSecondsToMilliseconds(50));
	}
}

function insaneReinforcementSpawn()
{
	const units = (!camClassicMode()) ? [cTempl.blokeheavy, cTempl.trikeheavy, cTempl.buggyheavy, cTempl.bjeepheavy] : [cTempl.bloke, cTempl.trike, cTempl.buggy, cTempl.bjeep];
	const limits = {minimum: 10, maxRandom: 6};
	const location = camMakePos("InsaneSpawnPos");
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_SCAV_7, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
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
		throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 13 : 10)),
		templates: (!camClassicMode()) ? [ cTempl.trikeheavy, cTempl.blokeheavy, cTempl.buggyheavy, cTempl.bjeepheavy ] : [ cTempl.trike, cTempl.bloke, cTempl.buggy, cTempl.bjeep ]
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
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.alpha5.pre, {
		area: "RTLZ",
		message: "C1-2_LZ",
		reinforcements: 60,
		retlz: true
	});

	const startPos = getObject("StartPosition");
	const lz = getObject("LandingZone");
	const tEnt = getObject("TransporterEntry");
	const tExt = getObject("TransporterExit");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	if (camClassicMode())
	{
		camSetArtifacts({
			"ScavLab": { tech: "R-Wpn-Mortar01Lt" },
			"NorthFactory": { tech: "R-Vehicle-Prop-Halftracks" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_scavengerRes, CAM_SCAV_7);

		camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.trike, (camAllowInsaneSpawns()) ? cTempl.trikeheavy : cTempl.triketwin, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.buggy, (camAllowInsaneSpawns()) ? cTempl.buggyheavy : cTempl.buggytwin, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.bjeep, (camAllowInsaneSpawns()) ? cTempl.bjeepheavy : cTempl.bjeeptwin, CAM_SCAV_7);

		camSetArtifacts({
			"ScavLab": { tech: "R-Wpn-Mortar01Lt" },
			"NorthFactory": { tech: ["R-Vehicle-Prop-Halftracks", "R-Wpn-Cannon1Mk1"] },
		});
	}

	camSetEnemyBases({
		"NorthGroup": {
			cleanup: "NorthBase",
			detectMsg: "C1-2_BASE1",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"WestGroup": {
			cleanup: "WestBase",
			detectMsg: "C1-2_BASE2",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"ScavLabGroup": {
			cleanup: "ScavLabCleanup",
			detectMsg: "C1-2_OBJ1",
		},
	});

	camDetectEnemyBase("ScavLabGroup");

	camSetFactories({
		"NorthFactory": {
			assembly: "NorthAssembly",
			order: CAM_ORDER_PATROL,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 20 : 15)),
			data: {
				pos: [
					camMakePos("NorthAssembly"),
					camMakePos("ScavLabPos"),
					camMakePos("RTLZ"),
				],
				interval: camSecondsToMilliseconds(20),
				regroup: false,
				count: -1,
			},
			group: camMakeGroup("NorthTankGroup"),
			templates: (!camClassicMode()) ? [ cTempl.trikeheavy, cTempl.blokeheavy, cTempl.buggyheavy, cTempl.bjeepheavy ] : [ cTempl.trike, cTempl.bloke, cTempl.buggy, cTempl.bjeep ]
		},
		"WestFactory": {
			assembly: "WestAssembly",
			order: CAM_ORDER_PATROL,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 13 : 10)),
			data: {
				pos: [
					camMakePos("WestAssembly"),
					camMakePos("GatesPos"),
					camMakePos("ScavLabPos"),
				],
				interval: camSecondsToMilliseconds(20),
				regroup: false,
				count: -1,
			},

			templates: (!camClassicMode()) ? [ cTempl.trikeheavy, cTempl.blokeheavy, cTempl.buggyheavy, cTempl.bjeepheavy ] : [ cTempl.trike, cTempl.bloke, cTempl.buggy, cTempl.bjeep ]
		},
	});

	queue("enableWestFactory", camChangeOnDiff(camSecondsToMilliseconds(30)));
}
