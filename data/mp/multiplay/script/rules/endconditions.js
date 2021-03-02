//The all logic of determining winners, losers and observers.
//From rules.js, the code from this file is not called.
//Everything fulfills through eventGameInit event in a separate namespace
//In rules.js, there was only one hack to save the minimap for the losers and observers function setMainReticule ().

namespace("co_");
const USERTYPE = {
	spectator: "spectator",
	player: {
		fighter: "fighter",
		winner: "winner",
		loser: "loser",
	},
};

// The time that the player's inactivity is allowed. Actions are considered
// - unit building
// - completion of the research
// - construction of base structures (factories, power plants, laboratories, modules and oil rigs)
// - dealing damage
const IDLETIME = 5*60*1000;

function inOneTeam(playnum, splaynum)
{
//	FIXME allianceExistsBetween dont correct if leave player in ALLIANCES_UNSHARED, ALLIANCES_TEAMS mode
//	and  team is garbage in NO_ALLIANCES, ALLIANCES mode
	if ((alliancesType == ALLIANCES_UNSHARED ||
		alliancesType == ALLIANCES_TEAMS) &&
 		(playerData[playnum].team != playerData[splaynum].team))
	{
		return false;
	}
	else if (alliancesType == NO_ALLIANCES && playnum != splaynum)
	{
		return false;
	}
	else if (alliancesType == ALLIANCES && !allianceExistsBetween(playnum, splaynum))
	{
		return false;
	}

	return true;
}

function hasFactory(playnum)
{
	let structs = [FACTORY, CYBORG_FACTORY, VTOL_FACTORY];

	for (let splaynum = 0; splaynum < maxPlayers; splaynum++)
		{
		if (!inOneTeam(playnum, splaynum))
		{
			continue;
		}

		for (let i = 0; i < structs.length; ++i)
		{
			let onMapStructss = enumStruct(splaynum, structs[i]);

			for (let j = 0; j < onMapStructss.length; ++j)
			{
				if (onMapStructss[j].status === BUILT)
				{
					return true;
				}
			}
		}
	}

	return false;
}

function hasDroid(playnum)
{
	for (let splaynum = 0; splaynum < maxPlayers; splaynum++)
	{
		if (inOneTeam(playnum, splaynum) && countDroid(DROID_ANY, splaynum) > 0)	// checking enemy player
		{
			return true;
		}
	}

	return false;
}

function hasOnlyConstructor(playnum)
{
	let count = 0;
	for (let splaynum = 0; splaynum < maxPlayers; splaynum++)
	{
		if (inOneTeam(playnum, splaynum))
		{
			count += countDroid(DROID_ANY, splaynum) - countDroid(DROID_CONSTRUCT, splaynum);
		}
	}
	if (count === 0) {return true;}
	else {return false;}
}

function canReachOil(playnum)
{
	let oils = enumFeature(ALL_PLAYERS).filter(function(e) {
		return e.stattype === OIL_RESOURCE;
	});

	for (let tplaynum = 0; tplaynum < maxPlayers; tplaynum++)
	{
		oils = oils.concat(enumStruct(tplaynum, "A0ResourceExtractor"));
	}

	for (let splaynum = 0; splaynum < maxPlayers; splaynum++)
	{
		if (!inOneTeam(playnum, splaynum))
		{
			continue;
		}

		if (enumStruct(splaynum, RESOURCE_EXTRACTOR).length != 0)
		{
			return true;
		}
		else
		{
			let trucks = enumDroid(splaynum, DROID_CONSTRUCT);

			for (let i = 0, len = trucks.length; i < len; ++i)
			{
				let truck = trucks[i];

				for (let j = 0, len2 = oils.length; j < len2; ++j)
				{
					let oil = oils[j];

					if (droidCanReach(truck, oil.x, oil.y))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

function activeGame(playnum)
{
	for (let splaynum = 0; splaynum < maxPlayers; splaynum++)
	{
		if (inOneTeam(playnum, splaynum) &&
		playerData[playnum].lastActivity + IDLETIME >= gameTime)
		{
			return true;
		}
	}
	return false;
}


function canPlay(playnum)
{
/*
	let feature = {
		factory: hasFactory(playnum),
		droid: hasDroid(playnum),
		onlyConstruct: hasOnlyConstructor(playnum),
		oilReach: canReachOil(playnum),
		activeGame: activeGame(playnum)
	};
	debug(playerData[playnum].name + JSON.stringify(feature));
*/

	if (!hasFactory(playnum) && !hasDroid(playnum))
	{
		return false;
	}
	else if (!hasFactory(playnum) && hasOnlyConstructor(playnum) && !canReachOil(playnum) )
	{
		return false;
	}
	else if (!activeGame(playnum))
	{
		return false;
	}
	return true;
}

function roomPlayability()
{
	let unPlayability = true;
	for (let playnum = 0; playnum < maxPlayers; playnum++)
	{
		for (let splaynum = 0; splaynum < maxPlayers; splaynum++)
		{
			if (playnum != splaynum &&
			playerData[playnum].usertype == USERTYPE.player.fighter &&
			playerData[splaynum].usertype == USERTYPE.player.fighter &&
			!inOneTeam (playnum, splaynum))
			{
				unPlayability = false;
			}
		}
	}
	if (unPlayability)
	{
//		debug(_("There are no opponents in the game."));
		console(_("There are no opponents in the game"));
//		gameOverMessage(true);
		return false;
	}
	else {return true;}
}

function toSpectator(playnum, remove)
{
	hackNetOff();
	setPower(0, playnum);
	let spotter = {
		X: mapWidth/2,
		Y: mapHeight/2,
		radius: Math.sqrt(mapWidth*mapWidth + mapHeight*mapHeight)/2*128
	};
	addSpotter(spotter.X, spotter.Y, playnum, spotter.radius, 0, 0);

	let HQstructs = enumStruct(playnum, HQ);
	HQstructs.forEach(function(e) {
		removeObject(e);
	});

	if (remove === true)
	{
		let droids = enumDroid(playnum, DROID_ANY);
		droids.forEach(function(e) {
			removeObject(e);
		});

		let structs = enumStruct(playnum);
		structs.forEach(function(e) {
			removeObject(e);
		});
	}

	if (selectedPlayer === playnum)
	{
		queue("hud", 100);
	}
	hackNetOn();
}

function hud()
{
	setMiniMap(true);
	setReticuleButton(4, "", "", "");
	setReticuleButton(5, "", "image_intelmap_up.png", "image_intelmap_down.png");
	setDesign(false);
	showInterface();
	hackPlayIngameAudio();
}

// /////////////////////////////////////////////////////////////////
// END CONDITIONS

function checkPlayerVictoryStatus()
{
	for (let playnum = 0; playnum < maxPlayers; playnum++)
	{
		if (playerData[playnum].usertype === USERTYPE.player.winner)
		{
			return; // When the winner is determined, nothing needs to be done.
		}
	}

	for (let playnum = 0; playnum < maxPlayers; playnum++)
	{
		if (playerData[playnum].usertype === USERTYPE.player.loser)
		{
			continue;
		}
		//we mark the retired players as lost and give them visibility. we save buildings
		else if (!canPlay(playnum) && (playerData[playnum].usertype != USERTYPE.spectator))
		{
			//See loss check in function canPlay()
			playerData[playnum].usertype = USERTYPE.player.loser;
			toSpectator(playnum, false);
			if  (selectedPlayer == playnum)
			{
				gameOverMessage(false);
			}
		}

		// Winning Conditions
		// all participants except teammates (this is not the same as the allies) cannot continue the game
	}

	let endGame = true;
	for (let playnum = 0; playnum < maxPlayers; playnum++)
	{
		if (playerData[playnum].usertype != USERTYPE.player.fighter)
		{
			continue;
		}

		for (let splaynum = 0; splaynum < maxPlayers; splaynum++)
		{
			if (inOneTeam(playnum, splaynum))
			{
				continue;
			}
			else if (playerData[splaynum].usertype === USERTYPE.player.fighter)
			{
				endGame = false;
			}
		}
	}

	if (endGame == true)
	{
		for (let playnum = 0; playnum < maxPlayers; playnum++)
		{
			if (playerData[playnum].usertype === USERTYPE.player.fighter)
			{
				playerData[playnum].usertype = USERTYPE.player.winner;
			}
		}

		if (playerData[selectedPlayer].usertype ===  USERTYPE.player.winner)
		{
			gameOverMessage(true);
		}
		if (playerData[selectedPlayer].usertype == USERTYPE.spectator)
		{
//			gameOverMessage(true);
			console(_("the battle is over, you can leave the room"));
//			debug("the battle is over, you can leave the room");
		}
	}
}

function co_eventGameInit()
{
	for (let playnum = 0; playnum < maxPlayers; playnum++)
	{
		playerData[playnum].lastActivity = gameTime;
			//we consider observers to players who cannot play from the beginning of the game
		if (!canPlay(playnum))
		{
			playerData[playnum].usertype = USERTYPE.spectator;
			toSpectator(playnum, true);
			continue;
		}
		else
		{
			playerData[playnum].usertype = USERTYPE.player.fighter;
		}

	}
	if (roomPlayability())
	{
		setTimer("checkPlayerVictoryStatus", 3000);
		setTimer("activityAlert", 10000);
	}
}

function activityAlert()
{
	if (playerData[selectedPlayer].usertype != USERTYPE.player.fighter)
	{
		setMissionTime(-1);
		removeTimer("activityAlert");
		return;
	}
	if (playerData[selectedPlayer].lastActivity + IDLETIME/2 < gameTime)
	{
		console(_("Playing passively will lead to defeat. Actions that are considered: - unit building - research completion - construction of base structures (factories, power plants, laboratories, modules and oil derricks) - dealing damage"));
	//		debug (getMissionTime());
		if (getMissionTime() > IDLETIME)
		{
			setMissionTime((playerData[selectedPlayer].lastActivity + IDLETIME - gameTime)/1000);
		}
	}
	if (playerData[selectedPlayer].lastActivity + IDLETIME/2 > gameTime)
	{
		setMissionTime(-1);
	}
}

function co_eventDroidBuilt (droid)
{
	if (droid.player != scavengerPlayer)
	{
		playerData[droid.player].lastActivity = gameTime;
	}
}
function co_eventStructureBuilt (structure)
{
	if (structure.player != scavengerPlayer)
	{
		playerData[structure.player].lastActivity = gameTime;
	}
}
function co_eventResearched (research, structure, player)
{
	if (player != scavengerPlayer)
	{
		playerData[player].lastActivity = gameTime;
	}
}
function co_eventAttacked (victim, attacker)
{
	if (attacker.player != scavengerPlayer)
	{
		playerData[attacker.player].lastActivity = gameTime;
	}
}
