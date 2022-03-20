function setupScavenger()	// inside hackNetOff()
{
	// Returns true if something is defined
	function isDefined(data)
	{
		return typeof data !== "undefined";
	}

	const derrick = "A0ResourceExtractor";
	const factory = "A0BaBaFactory";
	const vtolfac = "A0BaBaVtolFactory";
	const gen = "A0BaBaPowerGenerator";
	const repair = "ScavRepairCentre";
	const vtolpad = "A0BaBaVtolPad";

	const defenses = [
		"A0BaBaBunker",
		"A0BaBaBunker",
		"A0BaBaBunker",
		"A0CannonTower",
		"A0CannonTower",
		"A0CannonTower",
		"A0BaBaFlameTower",
		"A0BaBaFlameTower",
		"A0BaBaRocketPit",
		"A0BaBaRocketPit",
		"A0BaBaRocketPitAT",
		"A0BaBaMortarPit",
		"bbaatow",
	];

	const templates = [
		["B4body-sml-trike01","bTrikeMG"],
		["B4body-sml-trike01","bTrikeMG"],
		["B4body-sml-trike01","bTrikeMG"],
		["B4body-sml-trike01","bTrikeMG"],
		["B3body-sml-buggy01","BuggyMG"],
		["B3body-sml-buggy01","BuggyMG"],
		["B2JeepBody","BJeepMG"],
		["B2JeepBody","BJeepMG"],
		["B2JeepBody","BJeepMG"],
		["B3bodyRKbuggy01","BabaRocket"],
		["B3bodyRKbuggy01","BabaRocket"],
		["B2RKJeepBody","BabaRocket"],
		["B2RKJeepBody","BabaRocket"],
		["B4body-sml-trike01-Ultimate","bTrikeMG"],
		["B4body-sml-trike01-Ultimate","bTrikeMG"],
		["B4body-sml-trike01-Ultimate","bTrikeMG"],
		["B4body-sml-trike01-Ultimate","bTrikeMG"],
		["B3body-sml-buggy01-Ultimate","BuggyMG"],
		["B3body-sml-buggy01-Ultimate","BuggyMG"],
		["B2JeepBody-Ultimate","BJeepMG"],
		["B2JeepBody-Ultimate","BJeepMG"],
		["B2JeepBody-Ultimate","BJeepMG"],
		["B3bodyRKbuggy01-Ultimate","BabaRocket"],
		["B3bodyRKbuggy01-Ultimate","BabaRocket"],
		["B2RKJeepBody-Ultimate","BabaRocket"],
		["B2RKJeepBody-Ultimate","BabaRocket"],
		["BusBody","BusCannon"],
		["BusBody","BusCannon"],
		["BusBody","BabaPitRocketAT"],
		["B2tractor","BabaFlame"],
		["B2tractor","BabaFlame"],
		["B2tractor","BabaFlame"],
		["FireBody","BabaFlame"],
		["FireBody","BabaFlame"],
		["FireBody","BusCannon"],
		["FireBody","BabaPitRocket"],
		["FireBody","BabaPitRocketAT"],
		["ScavCamperBody","BabaPitRocket"],
		["ScavCamperBody","BusCannon"],
		["ScavTruckBody","BabaFlame","BabaRocket","BabaPitRocketAT"],
		["ScavTruckBody","BusCannon","BabaPitRocket","BabaRocket"],
		["ScavIcevanBody","BabaFlame"],
		["ScavIcevanBody","Mortar1Mk1"],
		["ScavNEXUStrack","ScavNEXUSlink"],
		["ScavNEXUStrack","ScavNEXUSlink"],
		["ScavNEXUStrack","ScavNEXUSlink"],
	];

	const vtolTemplates = [
		["ScavengerChopper", "MG1-VTOL-SCAVS"],
		["HeavyChopper", "Rocket-VTOL-Pod-SCAVS"],
	];

	for (const template of templates)
	{
		makeComponentAvailable(template[0], scavengerPlayer);
		makeComponentAvailable(template[1], scavengerPlayer);

		if (isDefined(template[2]))
		{
			makeComponentAvailable(template[2], scavengerPlayer);
		}

		if (isDefined(template[3]))
		{
			makeComponentAvailable(template[3], scavengerPlayer);
		}
	}

	for (const vtolTemplate of vtolTemplates)
	{

		makeComponentAvailable(vtolTemplate[0], scavengerPlayer);
		makeComponentAvailable(vtolTemplate[1], scavengerPlayer);

		if (isDefined(vtolTemplate[2]))
		{
			makeComponentAvailable(vtolTemplate[2], scavengerPlayer);
		}

		if (isDefined(vtolTemplate[3]))
		{
			makeComponentAvailable(vtolTemplate[3], scavengerPlayer);
		}

	}

	const SCAV_COMPONENTS = [
		"B4body-sml-trike01",
		"B3body-sml-buggy01",
		"B2JeepBody",
		"BusBody",
		"FireBody",
		"B1BaBaPerson01",
		"BaBaProp",
		"BaBaLegs",
		"bTrikeMG",
		"BuggyMG",
		"BJeepMG",
		"BusCannon",
		"BabaFlame",
		"BaBaMG",
		"B2crane1",
		"scavCrane1",
		"B2crane2",
		"scavCrane2",
		"ScavSensor",
		"Helicopter",
		"B2RKJeepBody",
		"B2tractor",
		"B3bodyRKbuggy01",
		"HeavyChopper",
		"ScavCamperBody",
		"ScavengerChopper",
		"ScavIcevanBody",
		"ScavNEXUSbody",
		"ScavNEXUStrack",
		"ScavTruckBody",
		"MG1-VTOL-SCAVS",
		"Rocket-VTOL-Pod-SCAVS",
		"ScavNEXUSlink",
		"BaBaCannon",
		"BabaPitRocket",
		"BabaPitRocketAT",
		"BabaRocket",
		"BTowerMG",
		"Mortar1Mk1",
		"B4body-sml-trike01-Ultimate",
		"B3body-sml-buggy01-Ultimate",
		"B2JeepBody-Ultimate",
		"B3bodyRKbuggy01-Ultimate",
		"B2RKJeepBody-Ultimate",
	];

	for (const scavComponent of SCAV_COMPONENTS)
	{
		makeComponentAvailable(scavComponent, scavengerPlayer);
	}

	enableStructure(factory, scavengerPlayer);
	enableStructure(vtolfac, scavengerPlayer);
	enableStructure(derrick, scavengerPlayer);
	enableStructure(gen, scavengerPlayer);
	enableStructure(repair, scavengerPlayer);
	enableStructure(vtolpad, scavengerPlayer);

	for (const defense of defenses)
	{
		enableStructure(defense, scavengerPlayer);
	}
}
