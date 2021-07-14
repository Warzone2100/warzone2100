// registrate events conditions_eventGameInit, conditions_eventDroidBuilt, conditions_eventStructureBuil, conditions_eventResearched, conditions_eventAttacked
namespace("conditions_");

const STATE_contender = "contender";
const STATE_winner = "winner";
const STATE_loser = "loser";

const STRUCTS = [FACTORY, CYBORG_FACTORY, VTOL_FACTORY]; // structures in which you can continue to play
// TODO remove HQ. The destruction of the HQ will hide the minimap.


// The time that the player's inactivity is allowed. Actions are considered
// - unit building
// - completion of the research
// - construction of base structures (factories, power plants, laboratories, modules and oil rigs)
// - dealing damage
const IDLETIME = 5 * 60 * 1000;
const BASESTRUCTS = {[FACTORY]: true, [CYBORG_FACTORY]: true, [VTOL_FACTORY]: true, [HQ]: true, [RESOURCE_EXTRACTOR]: true, [POWER_GEN]: true, [RESEARCH_LAB]: true};

const SPOTTER = {
	x: mapWidth / 2,
	y: mapHeight / 2,
	radius: (Math.sqrt(mapWidth * mapWidth + mapHeight * mapHeight) / 2) * 128,
};

var teams = []; // array class instance Team
var playersTeam = new Array(maxPlayers); // array class instancePlayer

class Player
{
	constructor(playNum)
	{
		this.playNum = playNum;
	}

	hasFactory()
	{
		for (let i = 0; i < STRUCTS.length; ++i)
		{
			const onMapStructss = enumStruct(this.playNum, STRUCTS[i]);
			for (let j = 0; j < onMapStructss.length; ++j)
			{
				if (onMapStructss[j].status === BUILT)
				{
					return true;
				}
			}
		}
		return false;
	}

	hasDroid()
	{
		if (countDroid(DROID_ANY, this.playNum) > 0)
		{
			return true;
		}
		return false;
	}

	hasOnlyConstructor()
	{
		if (countDroid(DROID_ANY, this.playNum) - countDroid(DROID_CONSTRUCT, this.playNum) === 0)
		{
			return true;
		}
		return false;
	}

	canReachOil()
	{
		if (enumStruct(this.playNum, RESOURCE_EXTRACTOR).length != 0)
		{
			return true;
		}

		let oils = enumFeature(ALL_PLAYERS).filter(function(e)
		{
			return e.stattype === OIL_RESOURCE;
		});
		for (let playnum = 0; playnum < maxPlayers; playnum++)
		{
			oils = oils.concat(enumStruct(playnum, "A0ResourceExtractor"));
		}

		const trucks = enumDroid(this.playNum, DROID_CONSTRUCT);
		for (let i = 0, len = trucks.length; i < len; ++i)
		{
			const truck = trucks[i];
			for (let j = 0, len2 = oils.length; j < len2; ++j)
			{
				const oil = oils[j];
				if (droidCanReach(truck, oil.x, oil.y))
				{
					return true;
				}
			}
		}
		return false;
	}

	openMap()
	{
		hackNetOff();
		addSpotter(SPOTTER.x, SPOTTER.y, this.playNum, SPOTTER.radius, 0, 0);
		hackNetOn();
	}

	hud(state)
	{
		if (this.playNum != selectedPlayer)
		{
			return;
		}
		if (state === STATE_loser)
		{
			gameOverMessage(false);
		}
		if (state === STATE_winner)
		{
			gameOverMessage(true);
		}
		setMiniMap(true); //minimap close after demolition HQ
	}
}


class Team
{
	constructor(playerPlayNums)
	{
		this.players = playerPlayNums.map(function(playNum)
		{
			return new Player(playNum);
		}); // array class instance  Player
		this.state = STATE_contender; // We don't have observers, so all teams have regular players.
		this.lastActivity = gameTime;
		playerPlayNums.forEach(
			(playerNum) =>
			{
				playersTeam[playerNum]= this;
			}
		);
	}

	activeGame()
	{
		if (this.lastActivity + IDLETIME >= gameTime)
		{
			return true;
		}
		return false;
	}

	hasFactory()
	{
		return this.players.some(
			(player) => {return player.hasFactory();}
		);
	}

	hasDroid()
	{
		return this.players.some(
			(player) => {return player.hasDroid();}
		);
	}

	hasOnlyConstructor()
	{
		return this.players.some(
			(player) => {return player.hasOnlyConstructor();}
		);
	}

	canReachOil()
	{
		return this.players.some(
			(player) => {return player.canReachOil();}
		);
	}

	canPlay()
	{ // TODO skip check if no new events.
		if (!this.activeGame())
		{
			return false;
		}
		if (!this.hasFactory() && !this.hasDroid())
		{
			return false;
		}
		if (!this.hasFactory() && this.hasOnlyConstructor() && !this.canReachOil())
		{
			return false;
		}
		return true;
	}

	updateState()
	{
		if (this.state == STATE_contender && !this.canPlay())
		{
			this.setState(STATE_loser);
		}
	}

	setState(state)
	{
		this.state = state;
		this.players.forEach(
			(player) =>
			{
				player.hud(this.state);
				player.openMap();
			}
		);
	}

	isContender()
	{
		return this.state == STATE_contender;
	}
}

function checkEndConditions()
{
	teams.forEach((team) =>
	{
		team.updateState();
	});
	const numTeamContender = teams.filter((team) =>
	{
		return team.isContender();
	}).length;
	if (numTeamContender == 1)
	{// game end
		teams.find((team) =>
		{
			return team.isContender();
		}).setState(STATE_winner);
	}
}


function inOneTeam(playnum, splaynum)
{
//	FIXME allianceExistsBetween dont correct if leave player in ALLIANCES_UNSHARED, ALLIANCES_TEAMS mode
//	and  team is garbage in NO_ALLIANCES, ALLIANCES mode
	if (
		(alliancesType == ALLIANCES_UNSHARED || alliancesType == ALLIANCES_TEAMS) &&
    playerData[playnum].team != playerData[splaynum].team
	)
	{
		return false;
	}
	else if (alliancesType == NO_ALLIANCES && playnum != splaynum)
	{
		return false;
	}
	else if (
		alliancesType == ALLIANCES &&
    !allianceExistsBetween(playnum, splaynum)
	)
	{
		return false;
	}

	return true;
}


function conditions_eventGameInit()
{
	const inTeamPlayNums = new Array(maxPlayers).fill(false);
	for (let playNum = 0; playNum < maxPlayers; playNum++)
	{
		if (inTeamPlayNums[playNum] != false)
		{
			continue;
		}
		inTeamPlayNums[playNum] = true;
		const members =[playNum];
		for (let splayNum = 0; splayNum < maxPlayers; splayNum++)
		{
			if ( inTeamPlayNums[splayNum] == false && inOneTeam(playNum, splayNum) == true)
			{
				members.push(splayNum);
				inTeamPlayNums[splayNum] = true;
			}
		}
		teams.push(new Team(members));
	}

	setTimer("activityAlert", 10*1000);
}

function activityAlert()
{
	if (playersTeam[selectedPlayer].state != STATE_contender)
	{
		setMissionTime(-1);
		removeTimer("activityAlert");
		return;
	}
	if (playersTeam[selectedPlayer].lastActivity + IDLETIME / 2 < gameTime)
	{
		console(
			_(
				"Playing passively will lead to defeat. Actions that are considered: - unit building - research completion - construction of base structures (factories, power plants, laboratories, modules and oil derricks) - dealing damage")
		);
		if (getMissionTime() > IDLETIME)
		{
			setMissionTime(
				(playersTeam[selectedPlayer].lastActivity + IDLETIME - gameTime) / 1000);
		}
	}
	if (playersTeam[selectedPlayer].lastActivity + IDLETIME / 2 > gameTime)
	{
		setMissionTime(-1); // remove timer widget
	}
}

function conditions_eventDroidBuilt(droid)
{
	if (droid.player != scavengerPlayer)
	{
		playersTeam[droid.player].lastActivity = gameTime;
	}
}
function conditions_eventStructureBuilt(structure)
{
	if (structure.player != scavengerPlayer && BASESTRUCTS[structure.stattype] == true)
	{
		playersTeam[structure.player].lastActivity = gameTime;
	}
}
function conditions_eventResearched(research, structure, player)
{
	if (player != scavengerPlayer)
	{
		playersTeam[player].lastActivity = gameTime;
	}
}
function conditions_eventAttacked(victim, attacker)
{
	if (attacker.player != scavengerPlayer)
	{
		playersTeam[attacker.player].lastActivity = gameTime;
	}
}
