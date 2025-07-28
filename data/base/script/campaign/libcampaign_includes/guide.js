
////////////////////////////////////////////////////////////////////////////////
// Guide topics management.
////////////////////////////////////////////////////////////////////////////////

function __camDoAddVTOLUseTopicsImpl()
{
	addGuideTopic("wz2100::units::propulsions::vtols::defending");
	addGuideTopic("wz2100::units::propulsions::vtols::attacking", SHOWTOPIC_FIRSTADD);
}

function __camDoAddCommanderUseTopicsImpl()
{
	addGuideTopic("wz2100::units::commanders::targeting");
	addGuideTopic("wz2100::units::commanders::detaching");
	addGuideTopic("wz2100::units::commanders::repairs");
	addGuideTopic("wz2100::units::commanders::attaching", SHOWTOPIC_FIRSTADD);
}

function __camDoAddVTOLDefenseTopicsImpl()
{
	addGuideTopic("wz2100::units::propulsions::vtols::defendingagainst", SHOWTOPIC_FIRSTADD);
}

function __camDoAddVTOLDefenseTopics()
{
	// delayed by a bit so the player hopefully has a solid chance to notice the VTOLs
	queue("__camDoAddVTOLDefenseTopicsImpl", camSecondsToMilliseconds(2));
}

function __camDoAddVTOLUseTopics()
{
	queue("__camDoAddVTOLUseTopicsImpl", camSecondsToMilliseconds(1));
}

function __camDoAddCommanderUseTopics()
{
	queue("__camDoAddCommanderUseTopicsImpl", camSecondsToMilliseconds(1));
}

function __camGuideTopicCheckResearchComplete(targetResearchName, justResearchedObj = null)
{
	if (justResearchedObj && justResearchedObj.name == targetResearchName)
	{
		return true;
	}
	else if (justResearchedObj == null)
	{
		const res = getResearch(targetResearchName);
		if (res && res.done)
		{
			return true;
		}
	}
	return false;
}

function __camProcessResearchGatedGuideTopics(research = null)
{
	const __SHOW_FLAGS = (research == null) ? 0 : SHOWTOPIC_FIRSTADD;
	// First "different" weapon type
	if (__camGuideTopicCheckResearchComplete('R-Wpn-Flamer01Mk1', research))
	{
		// in this case, the category topic must come first (as adding the child topics will also add it, but we want the category topic to be what's displayed)
		addGuideTopic("wz2100::weapons", __SHOW_FLAGS);
		addGuideTopic("wz2100::weapons::antipersonnel");
		addGuideTopic("wz2100::weapons::flamer");
		addGuideTopic("wz2100::units::thermaldamage");
	}
	// Repair Turret researched
	if (__camGuideTopicCheckResearchComplete('R-Sys-MobileRepairTurret01', research))
	{
		addGuideTopic("wz2100::units::repairing", __SHOW_FLAGS);
	}
	// First Module researched
	if (__camGuideTopicCheckResearchComplete('R-Struc-PowerModuleMk1', research))
	{
		addGuideTopic("wz2100::structures::modules", __SHOW_FLAGS);
	}
	// Sensor Turret researched
	if (__camGuideTopicCheckResearchComplete('R-Sys-Sensor-Turret01', research))
	{
		// in this case, the category topic must come first (as adding the child topics will also add it, but we want the category topic to be what's displayed)
		addGuideTopic("wz2100::units::sensors", __SHOW_FLAGS);
		addGuideTopic("wz2100::units::sensors::unassigning");
		addGuideTopic("wz2100::units::sensors::using");
	}
	// First "All-Rounder" weapon researched
	if (__camGuideTopicCheckResearchComplete('R-Wpn-Cannon1Mk1', research))
	{
		addGuideTopic("wz2100::weapons::allrounder", __SHOW_FLAGS);
	}
	// First "Artillery" weapon researched
	if (__camGuideTopicCheckResearchComplete('R-Wpn-Mortar01Lt', research))
	{
		addGuideTopic("wz2100::weapons::artillery", __SHOW_FLAGS);
	}
	// Half-Track Propulsion researched
	if (__camGuideTopicCheckResearchComplete('R-Vehicle-Prop-Halftracks', research))
	{
		addGuideTopic("wz2100::units::propulsions::wheels");
		addGuideTopic("wz2100::units::propulsions::halftracks", __SHOW_FLAGS);
	}
	// First "Anti-Tank" weapon researched
	if (__camGuideTopicCheckResearchComplete('R-Wpn-Rocket05-MiniPod', research))
	{
		addGuideTopic("wz2100::weapons::antitank", __SHOW_FLAGS);
	}
	// Tracked Propulsion researched
	if (__camGuideTopicCheckResearchComplete('R-Vehicle-Prop-Tracks', research))
	{
		addGuideTopic("wz2100::units::propulsions::tracks", __SHOW_FLAGS);
	}
	// First (and only) Bunker Buster weapon researched
	if (__camGuideTopicCheckResearchComplete('R-Wpn-Rocket03-HvAT', research))
	{
		addGuideTopic("wz2100::weapons::bunkerbuster", __SHOW_FLAGS);
	}
	// First "Anti-Air" weapon type
	if (__camGuideTopicCheckResearchComplete('R-Wpn-AAGun03', research))
	{
		addGuideTopic("wz2100::weapons::antiair", __SHOW_FLAGS);
	}
	// Command Turret researched
	if (__camGuideTopicCheckResearchComplete('R-Comp-CommandTurret01', research))
	{
		// in this case, the category topic must come first (as adding the child topics will also add it, but we want the category topic to be what's displayed)
		addGuideTopic("wz2100::units::commanders", __SHOW_FLAGS);
	}
	// Repair Facility researched
	if (__camGuideTopicCheckResearchComplete('R-Struc-RepairFacility', research))
	{
		addGuideTopic("wz2100::structures::repairfacility", __SHOW_FLAGS);
	}
	// VTOL Factory researched
	if (__camGuideTopicCheckResearchComplete('R-Struc-VTOLFactory', research))
	{
		addGuideTopic("wz2100::structures::vtolfactory", __SHOW_FLAGS);
	}
	// Cyborgs
	if (__camGuideTopicCheckResearchComplete('R-Struc-Factory-Cyborg', research))
	{
		addGuideTopic("wz2100::units::propulsions::cyborg");
		addGuideTopic("wz2100::structures::cyborgfactory", __SHOW_FLAGS);
	}
	// Hover Propulsion researched
	if (__camGuideTopicCheckResearchComplete('R-Vehicle-Prop-Hover', research))
	{
		addGuideTopic("wz2100::units::propulsions::hover", __SHOW_FLAGS);
	}
	// VTOL Propulsion researched
	if (__camGuideTopicCheckResearchComplete('R-Vehicle-Prop-VTOL', research))
	{
		addGuideTopic("wz2100::units::propulsions::vtols::weapons");
		addGuideTopic("wz2100::units::propulsions::vtols::using", __SHOW_FLAGS);
	}
	// CB Turret researched
	if (__camGuideTopicCheckResearchComplete('R-Sys-CBSensor-Turret01', research))
	{
		addGuideTopic("wz2100::units::sensors::cb", __SHOW_FLAGS);
	}
	// First (explosive) bomb researched
	if (__camGuideTopicCheckResearchComplete("R-Wpn-Bomb01", research))
	{
		addGuideTopic("wz2100::weapons::bombs::explosive", __SHOW_FLAGS);
	}
	// First incendiary bomb researched
	if (__camGuideTopicCheckResearchComplete("R-Wpn-Bomb03", research))
	{
		addGuideTopic("wz2100::weapons::bombs::incendiary", __SHOW_FLAGS);
	}
	// First defense researched
	if (__camGuideTopicCheckResearchComplete("R-Defense-Tower01", research))
	{
		addGuideTopic("wz2100::general::defendingbase", __SHOW_FLAGS);
	}
}

function __camEnableGuideTopics()
{
	// Always enable (most) general topics & unit orders
	addGuideTopic("wz2100::general::**", 0, ["wz2100::general::artifacts", "wz2100::general::researching", "wz2100::general::power", "wz2100::general::defendingbase"]);
	addGuideTopic("wz2100::unitorders::**");
	// Basic base / structure topics
	addGuideTopic("wz2100::structures::building");
	addGuideTopic("wz2100::structures::demolishing");
	const __IS_ALPHA_ONE = (__camNextLevel === cam_levels.alpha2);
	if (!__IS_ALPHA_ONE) // If not very first original campaign level (CAM1A), which displays these in response to events, add them
	{
		addGuideTopic("wz2100::general::artifacts");
		addGuideTopic("wz2100::general::researching");
		addGuideTopic("wz2100::general::power");
		addGuideTopic("wz2100::general::defendingbase");
		// Basic structure topics
		addGuideTopic("wz2100::structures::hq");
		addGuideTopic("wz2100::structures::researchfacility");
		addGuideTopic("wz2100::structures::oilderrick");
		addGuideTopic("wz2100::structures::powergenerator");
		addGuideTopic("wz2100::structures::rallypoint");
		addGuideTopic("wz2100::structures::factory");
		// Basic units topics
		addGuideTopic("wz2100::units::building");
		addGuideTopic("wz2100::units::designing");
	}
	// Handle research-driven topics (for things already researched - i.e. on savegame load or starting a later campaign)
	__camProcessResearchGatedGuideTopics();
	// Handle built-unit triggered topics
	if (countDroid(DROID_COMMAND, CAM_HUMAN_PLAYER) > 0)
	{
		addGuideTopic("wz2100::units::commanders::**");
	}
	let foundDroids_VTOL = false;
	const droids = enumDroid(CAM_HUMAN_PLAYER);
	for (let x = 0, len = droids.length; x < len; ++x)
	{
		const droid = droids[x];
		if (droid.isVTOL)
		{
			foundDroids_VTOL = true;
			break; // if checking for anything else in the future, remove this
		}
	}
	if (foundDroids_VTOL)
	{
		addGuideTopic("wz2100::units::propulsions::vtols::**");
	}
}

function __camEnableGuideTopicsForTransport(transport)
{
	const droids = enumCargo(transport);
	for (let i = 0, len = droids.length; i < len; ++i)
	{
		const droid = droids[i];
		if (droid.droidType === DROID_COMMAND)
		{
			addGuideTopic("wz2100::units::commanders::**");
		}
	}
}
