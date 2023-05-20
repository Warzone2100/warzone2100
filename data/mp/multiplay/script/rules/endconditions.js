// registrate events conditions_eventGameInit, conditions_eventDroidBuilt, conditions_eventStructureBuil, conditions_eventResearched, conditions_eventAttacked
namespace("conditions_");

const STATE_contender = "contender";
const STATE_winner = "winner";
const STATE_loser = "loser";
const STATE_spectator = "spectator";
const STRUCTS = [FACTORY, CYBORG_FACTORY, VTOL_FACTORY]; // structures in which you can continue to play

// Uses global var: ```idleTime```
// The time that the player's inactivity is allowed. Actions are considered
// - unit building
// - completion of the research
// - construction of base structures (factories, power plants, laboratories, modules and oil rigs)
// - dealing damage
const BASESTRUCTS = [FACTORY, CYBORG_FACTORY, VTOL_FACTORY, HQ, RESOURCE_EXTRACTOR, POWER_GEN, RESEARCH_LAB];

var ENABLE_activity;
var teams; // array class instance Team
var playersTeam; // array class instancePlayer

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
		if (enumStruct(this.playNum, RESOURCE_EXTRACTOR).length > 0)
		{
			return true;
		}

		let oils = enumFeature(ALL_PLAYERS).filter((e) => (
			e.stattype === OIL_RESOURCE
		));
		for (let playnum = 0; playnum < maxPlayers; ++playnum)
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

	finalizeGame(state)
	{
		const transformToSpectator = (state === STATE_loser && !isSpectator(this.playNum) && playerData[this.playNum].isHuman);
		if (state === STATE_loser && this.playNum === selectedPlayer)
		{
			gameOverMessage(false);
		}
		if (state === STATE_winner && this.playNum === selectedPlayer)
		{
			gameOverMessage(true);
		}
		if (transformToSpectator)
		{
			// should come *after* the gameOverMessage call to ensure the proper gameOverMessage is displayed
			transformPlayerToSpectator(this.playNum);
		}
	}
}

class Team
{
	constructor(playerPlayNums)
	{
		this.players = playerPlayNums.map((playNum) => (new Player(playNum))); // array class instance  Player
		this.lastActivity = gameTime;
		this.onlyAIbots = playerPlayNums.some((playNum) => (
			playerData[playNum].isAI === true
		));
		playerPlayNums.forEach((playerNum) => {
			playersTeam[playerNum] = this;
		});
	}

	activeGame()
	{
		if (this.lastActivity + idleTime >= gameTime)
		{
			return true;
		}
		if (this.onlyAIbots)
		{
			// don't boot teams that are made up of only AI bots
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

	canPlay() // TODO skip check if no new events.
	{
		if (!this.activeGame() && ENABLE_activity)
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

	containsPlayer(playNum)
	{
		return this.players.some(
			(player) => {return player.playNum === playNum;}
		);
	}

	setState(state)
	{
		this.state = state;
		if (state ===  STATE_winner || state === STATE_loser ||  state === STATE_spectator)
		{
			this.players.forEach(
				(player) =>
				{
//					debug("Setting player " + player.playNum + " to state: " + this.state);
					player.finalizeGame(this.state);
				}
			);
		}
	}

	isContender()
	{
		return this.state === STATE_contender;
	}
}

function checkEndConditions()
{
	const newlyLosingTeams = teams.filter((team) => (
		team.isContender() && !team.canPlay()
	));
	newlyLosingTeams.forEach((team) => {
		// inform other players if this team / player lost by virtue of being inactive
		if (!team.activeGame() && ENABLE_activity)
		{
			team.players.forEach((player) => {
				console(
					_("Player")
					+ player.playNum + ": " +
					_("Lost due to being inactive / playing passively.")
				);
			});
		}

		// set to loser status
		team.setState(STATE_loser);
	});
	const contenderTeams = teams.filter((team) => (team.isContender()));
	if (contenderTeams.length <= 1 && newlyLosingTeams.length > 0) // game end
	{
		contenderTeams.forEach((team) => {
			team.setState(STATE_winner);
		});

		// game over message for selectedPlayer who is spectator
		// (can be spectator-only slots who have not yet received a message,
		// or previous losers who were converted to spectators who should now receive
		// a new message that the game has fully ended)
		if (isSpectator(-1) && !newlyLosingTeams.some((team) => (team.containsPlayer(selectedPlayer))))
		{
			gameOverMessage(false);
		}
	}
}

//	FIXME allianceExistsBetween() dont correct if leave player in ALLIANCES_UNSHARED, ALLIANCES_TEAMS mode
//	and  team is garbage in NO_ALLIANCES, ALLIANCES mode
function inOneTeam(playnum, splaynum)
{

	if (
		(alliancesType === ALLIANCES_UNSHARED || alliancesType === ALLIANCES_TEAMS) &&
	playerData[playnum].team === playerData[splaynum].team
	)
	{
		return true;
	}
	else if (alliancesType === NO_ALLIANCES && playnum === splaynum)
	{
		return true;
	}
	//Victory in alliance mode is also personal.
	//Alliances do not affect victory.
	//allianceExistsBetween() is not used.
	else if (alliancesType === ALLIANCES && playnum === splaynum)
	{
		return true;
	}
	return false;
}

function createTeams()
{
	teams = [];
	playersTeam = Array(maxPlayers);
	const inTeamPlayNums = Array(maxPlayers).fill(false);
	for (let playNum = 0; playNum < maxPlayers; ++playNum)
	{
		if (isSpectator(playNum))
		{
			continue; // skip slots that start as spectators
		}
		if (!playerData[playNum].isHuman && !playerData[playNum].isAI)
		{
			// not an allocated slot (is closed or no player / AI)
			continue;
		}
		if (inTeamPlayNums[playNum] === true)
		{
			continue;
		}
		inTeamPlayNums[playNum] = true;
		const members =[playNum];
		for (let splayNum = 0; splayNum < maxPlayers; ++splayNum)
		{
			if (isSpectator(splayNum))
			{
				continue; // skip slots that start as spectators
			}
			if (!playerData[splayNum].isHuman && !playerData[splayNum].isAI)
			{
				// not an allocated slot (is closed or no player / AI)
				continue;
			}
			if (inTeamPlayNums[splayNum] === false && inOneTeam(playNum, splayNum) === true)
			{
				members.push(splayNum);
				inTeamPlayNums[splayNum] = true;
			}
		}
		const team = new Team(members);
		if (team.canPlay())
		{
			teams.push(team);
			team.setState(STATE_contender);
		}
		else
		{
			team.setState(STATE_spectator);
		}
	}

}

/////////////////////////////////////
//First start and loading the save.//
/////////////////////////////////////

function conditions_eventGameInit()
{
	ENABLE_activity = (challenge != true && isMultiplayer === true && idleTime > 0); //The prohibition on passive play can interfere when playing against bots. There is no reason to end a fight earlier in PVE.
	createTeams();
	//find old type spectators
	if  (ENABLE_activity && !isSpectator(-1))
	{
		setTimer("activityAlert", 10*1000);
	}
	setTimer("checkEndConditions", 3000);
}

function conditions_eventGameLoaded()
{
	createTeams();
}

///////////////////////////////////////////////////////////////////////////
//Logging for active actions and displaying warnings during passive play.//
///////////////////////////////////////////////////////////////////////////

function activityAlert()
{
	// avoid using selectedPlayer to access playersTeam array if it's a spectator (as it may be beyond the bounds of playersTeam.length for spectator-only slots)
	if (isSpectator(-1) || (playersTeam[selectedPlayer].state !== STATE_contender))
	{
		setMissionTime(-1);
		removeTimer("activityAlert");
		return;
	}
	if (playersTeam[selectedPlayer].lastActivity + idleTime / 2 < gameTime)
	{
		console(
			_("Playing passively will lead to defeat. Actions that are considered:")
		);
		console(
			_("- unit building - research completion - construction of base structures (factories, power plants, laboratories, modules and oil derricks) - dealing damage")
		);
		if (getMissionTime() > idleTime)
		{
			setMissionTime(
				(playersTeam[selectedPlayer].lastActivity + idleTime - gameTime) / 1000);
		}
	}
	if (playersTeam[selectedPlayer].lastActivity + idleTime / 2 > gameTime)
	{
		setMissionTime(-1); // remove timer widget
	}
}
function conditions_eventDroidBuilt(droid)
{
	if (droid.player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (playersTeam[droid.player])
	{
		playersTeam[droid.player].lastActivity = gameTime;
	}
}
function conditions_eventStructureBuilt(structure)
{
	if (structure.player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (BASESTRUCTS.includes(structure.stattype) === true && playersTeam[structure.player])
	{
		playersTeam[structure.player].lastActivity = gameTime;
	}
}
function conditions_eventResearched(research, structure, player)
{
	if (player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (playersTeam[player])
	{
		playersTeam[player].lastActivity = gameTime;
	}
}
function conditions_eventAttacked(victim, attacker)
{
	if (attacker.player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (playersTeam[attacker.player] && playersTeam[attacker.player] != playersTeam[victim.player])
	{
		playersTeam[attacker.player].lastActivity = gameTime;
	}
}
