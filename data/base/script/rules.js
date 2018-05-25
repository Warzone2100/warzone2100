// General rules for the campaign
//
// * Enable unit design and minimap only when an HQ exists
receiveAllEvents(true); //Needed to allow enemy research to apply to them

include("script/weather.js");

var mainReticule = false;
var allowDesign = false;
const CREATE_LIKE_EVENT = 0;
const DESTROY_LIKE_EVENT = 1;
const TRANSFER_LIKE_EVENT = 2;

function setMainReticule()
{
	var offHQ = enumStructOffWorld(selectedPlayer, "A0CommandCentre");
	var offRes = enumStructOffWorld(selectedPlayer, "A0ResearchFacility");
	var offCr = enumStructOffWorld(selectedPlayer, "A0ComDroidControl");
	var fCount = 0;
	var facs = ["A0LightFactory", "A0CyborgFactory", "A0VTolFactory1"];

	for (var i = 0, l = facs.length; i < l; ++i)
	{
		var f = facs[i];
		var offWorldCount = enumStructOffWorld(selectedPlayer, f);
		fCount = fCount + countStruct(f) + (offWorldCount !== null ? offWorldCount.length : 0);
		if (fCount > 0)
		{
			break; //found a factory
		}
	}

	setReticuleButton(0, _("Close"), "image_cancel_up.png", "image_cancel_down.png");
	if (fCount > 0 && getMissionType() !== LDS_EXPAND_LIMBO)
	{
		setReticuleButton(1, _("Manufacture (F1)"), "image_manufacture_up.png", "image_manufacture_down.png");
	}
	else
	{
		setReticuleButton(1, _("Manufacture - build factory first"), "", "");
	}
	if (countStruct("A0ResearchFacility") + (offRes !== null ? offRes.length : 0) > 0 && getMissionType() !== LDS_EXPAND_LIMBO)
	{
		setReticuleButton(2, _("Research (F2)"), "image_research_up.png", "image_research_down.png");
	}
	else
	{
		setReticuleButton(2, _("Research - build research facility first"), "", "");
	}
	if (enumDroid(selectedPlayer, DROID_CONSTRUCT).length > 0)
	{
		setReticuleButton(3, _("Build (F3)"), "image_build_up.png", "image_build_down.png");
	}
	else
	{
		setReticuleButton(3, _("Build - manufacture constructor droids first"), "", "");
	}
	if (allowDesign === true || (offHQ !== null ? offHQ.length > 0 : false))
	{
		setReticuleButton(4, _("Design (F4)"), "image_design_up.png", "image_design_down.png");
	}
	else
	{
		setReticuleButton(4, _("Design - construct HQ first"), "", "");
	}
	setReticuleButton(5, _("Intelligence Display (F5)"), "image_intelmap_up.png", "image_intelmap_down.png");
	if (enumDroid(selectedPlayer, DROID_COMMAND).length > 0 && countStruct("A0ComDroidControl") + (offCr !== null ? offCr.length : 0) > 0)
	{
		setReticuleButton(6, _("Commanders (F6)"), "image_commanddroid_up.png", "image_commanddroid_down.png");
	}
	else
	{
		setReticuleButton(6, _("Commanders - manufacture commanders first"), "", "");
	}
}

function reticuleUpdate(obj, eventType)
{
	var update_reticule = false;
	var tryUpdate = (obj.player === selectedPlayer || eventType === TRANSFER_LIKE_EVENT);

	if (tryUpdate && obj.type === STRUCTURE)
	{
		if (obj.stattype === HQ)
		{
			var flag = (countStruct("A0CommandCentre", selectedPlayer) > 0);
			//See if one is on the home map
			if (flag === false)
			{
				var offHQ = enumStructOffWorld(selectedPlayer, "A0CommandCentre");
				flag = (offHQ !== null ? offHQ.length > 0 : false);
			}
			setMiniMap(flag); // show minimap or not
			setDesign(flag); // permit designs or not
			allowDesign = flag;
			update_reticule = true;
		}

		if (obj.stattype === RESEARCH_LAB || obj.stattype === CYBORG_FACTORY ||
			obj.stattype === VTOL_FACTORY || obj.stattype === FACTORY || obj.stattype === COMMAND_CONTROL)
		{
			update_reticule = true;
		}
	}
	else if (tryUpdate && obj.type === DROID)
	{
		if (obj.droidType === DROID_CONSTRUCT || obj.droidType === DROID_COMMAND ||
			obj.droidType === DROID_TRANSPORTER || obj.droidType === DROID_SUPERTRANSPORTER)
		{
			update_reticule = true;
		}
	}

	if (mainReticule && update_reticule)
	{
		//Wait a tick for the counts to update
		const TICK_TIME = 100;
		queue("setMainReticule", TICK_TIME);
	}
}

function setupGame()
{
	if (tilesetType === "ARIZONA")
	{
		setCampaignNumber(1);
	}
	else if (tilesetType === "URBAN")
	{
		setCampaignNumber(2);
		replaceTexture("page-7-barbarians-arizona.png", "page-7-barbarians-urban.png");
		replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-urban.png");
		replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-urban.png");
	}
	else if (tilesetType === "ROCKIES")
	{
		setCampaignNumber(3);
		replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-rockies.png");
		replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-rockies.png");
		replaceTexture("page-7-barbarians-arizona.png", "page-7-barbarians-kevlar.png");
	}

	if (tilesetType !== "ARIZONA")
	{
		setSky("texpages/page-25-sky-urban.png", 0.5, 10000.0);
	}

	// Disable by default
	setMiniMap(false);
	setDesign(false);

	allowDesign = (countStruct("A0CommandCentre", selectedPlayer) > 0);
	if (allowDesign === false)
	{
		var offHQ = enumStructOffWorld(selectedPlayer, "A0CommandCentre");
		allowDesign = (offHQ !== null ? offHQ.length > 0 : false);
	}

	setMiniMap(allowDesign);
	setDesign(allowDesign);

	setMainReticule();
	showInterface();
	hackPlayIngameAudio();
	mainReticule = true;
	queue("resetPower", 1000);
}

function eventGameLoaded()
{
	setupGame();
}

function eventGameInit()
{
	setupGame();
}

function setLimits()
{
	setDroidLimit(selectedPlayer, 100, DROID_ANY);
	setDroidLimit(selectedPlayer, 10, DROID_COMMAND);
	setDroidLimit(selectedPlayer, 15, DROID_CONSTRUCT);

	for (var i = 0; i < maxPlayers; ++i)
	{
		setStructureLimits("A0PowerGenerator", 5, i);
		setStructureLimits("A0ResourceExtractor", 200, i);
		setStructureLimits("A0ResearchFacility", 5, i);
		setStructureLimits("A0LightFactory", 5, i);
		setStructureLimits("A0CyborgFactory", 5, i);
		setStructureLimits("A0VTolFactory1", 5, i);
		//non human players get five of these
		setStructureLimits("A0CommandCentre", i === selectedPlayer ? 1 : 5, i);
		setStructureLimits("A0ComDroidControl", i === selectedPlayer ? 1 : 5, i);
		setStructureLimits("A0CommandCentreNP", 5, i);
		setStructureLimits("A0CommandCentreCO", 5, i);
		setStructureLimits("A0CommandCentreNE", 5, i);
	}
}

function resetPower()
{
	var powerLimit = 999999;
	var powerProductionRate = 100;

	// set income modifier/power storage for player 0 (human)
	if (difficulty === EASY)
	{
		powerProductionRate = 115;
	}
	else if (difficulty === HARD)
	{
		powerProductionRate = 85;
		powerLimit = 20000; //base value for Alpha

		if (tilesetType === "URBAN")
		{
			powerLimit = powerLimit + 5000;
		}
		else if (tilesetType === "ROCKIES")
		{
			powerLimit = powerLimit + 10000;
		}
	}
	else if (difficulty === INSANE)
	{
		powerProductionRate = 70;
		powerLimit = 12000; //base value for Alpha

		if (tilesetType === "URBAN")
		{
			powerLimit = powerLimit + 2000;
		}
		else if (tilesetType === "ROCKIES")
		{
			powerLimit = powerLimit + 4000;
		}
	}

	setPowerModifier(powerProductionRate);
	setPowerStorageMaximum(powerLimit);
	if (playerPower(selectedPlayer) >= powerLimit)
	{
		setPower(powerLimit - 1, selectedPlayer);
	}
}

function eventStartLevel()
{
	setLimits();
	if (tilesetType === "URBAN" || tilesetType === "ROCKIES")
	{
		weatherCycle();
		setTimer("weatherCycle", 45000);
	}
}

function eventDroidBuilt(droid, structure)
{
	if (droid.player === selectedPlayer)
	{
		reticuleUpdate(droid, CREATE_LIKE_EVENT);
	}
}

function eventStructureBuilt(struct)
{
	if (struct.player === selectedPlayer)
	{
		reticuleUpdate(struct, CREATE_LIKE_EVENT);
	}
}

function eventDestroyed(victim)
{
	if (victim.player === selectedPlayer)
	{
		reticuleUpdate(victim, DESTROY_LIKE_EVENT);
	}
}

function eventObjectTransfer(obj, from)
{
	reticuleUpdate(obj, TRANSFER_LIKE_EVENT);
}

//Could be the last remaining trucks are on it.
function eventTransporterLanded(transport)
{
	if (transport.player === selectedPlayer)
	{
		reticuleUpdate(transport, TRANSFER_LIKE_EVENT);
	}
}

function eventResearched(research, structure, player)
{
	//debug("RESEARCH : " + research.fullname + "(" + research.name + ") for " + player);
	// iterate over all results
	for (var i = 0; i < research.results.length; i++)
	{
		var v = research.results[i];
		//debug("    RESULT : class=" + v['class'] + " parameter=" + v['parameter'] + " value=" + v['value'] + " filter=" + v['filterParameter'] + " filterparam=" + v['filterParameter']);
		for (var cname in Upgrades[player][v['class']]) // iterate over all components of this type
		{
			var parameter = v['parameter'];
			var ctype = v['class'];
			var filterparam = v['filterParameter'];
			if ('filterParameter' in v && Stats[ctype][cname][filterparam] != v['filterValue']) // more specific filter
			{
				continue;
			}
			if (Stats[ctype][cname][parameter] > 0) // only applies if stat has above zero value already
			{
				Upgrades[player][ctype][cname][parameter] += Math.ceil(Stats[ctype][cname][parameter] * v['value'] / 100);
				//debug("      upgraded " + cname + " to " + Upgrades[player][ctype][cname][parameter] + " by " + Math.ceil(Stats[ctype][cname][parameter] * v['value'] / 100));
			}
		}
	}
}

var lastHitTime = 0;
function eventAttacked(victim, attacker)
{
	if ((victim.player === selectedPlayer) && gameTime > lastHitTime + 5000)
	{
		lastHitTime = gameTime;
		if (victim.type === STRUCTURE)
		{
			playSound("pcv337.ogg", victim.x, victim.y, victim.z);
		}
		else
		{
			playSound("pcv399.ogg", victim.x, victim.y, victim.z);
		}
	}
}
