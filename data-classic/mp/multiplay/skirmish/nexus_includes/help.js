
function defendingOwnBase()
{
	return (helpingAlly() && helpInfo.lastHelpPlayer === me);
}

function haveBeacon(player)
{
	if (!defined(player))
	{
		debugMessage("haveBeacon. Undefined player parameter.");
		return false;
	}

	return (helpInfo[player].lastHelpTime > 0 && !beaconTimeout(player));
}

//See if the last beacon from a player was placed long ago.
function beaconTimeout(player)
{
	if (!defined(player))
	{
		debugMessage("beaconTimeout. Undefined player parameter.");
		return false;
	}

	if (!defined(helpInfo[player].lastHelpTime))
	{
		return false;
	}

	return (helpInfo[player].lastHelpTime > 0 && (helpInfo[player].lastHelpTime + BEACON_TIMEOUT < gameTime));
}

function stopDefendingLocation()
{
	defendInfo.location.x = undefined;
	defendInfo.location.y = undefined;
	defendInfo.timeout = undefined;
	defendInfo.startTime = undefined;
	defendInfo.moveType = undefined;
	defendInfo.busy = false;

	orderGroupLoc(groups.attackers, BASE.x, BASE.y, DORDER_SCOUT);
}

function defendLocationTimeout()
{
	return (defined(defendInfo.timeout) && defendInfo.timeout < gameTime);
}

function defendingLocation()
{
	return (defined(defendInfo.location.x) && defined(defendInfo.location.y));
}

//move only ever is DORDER_MOVE when it is 'me' defending my base.
function defendLocation(x, y, timeout, move)
{
	var radius = 7; //was 15

	defendInfo.location.x = x;
	defendInfo.location.y = y;
	defendInfo.startTime = gameTime;
	defendInfo.timeout = timeout;
	defendInfo.moveType = move ? DORDER_MOVE : DORDER_SCOUT;

	if (enumGroup(groups.attackers).length > 0)
	{
		var avg = groupCoordinateAverage(groups.attackers);

		if (distBetweenTwoPoints(avg.x, avg.y, x, y) > radius)
		{
			orderGroupLoc(groups.attackers, x, y, defendInfo.moveType);
		}
	}
}

function stopHelpingAlly()
{
	var ally = helpInfo.lastHelpPlayer;

	helpInfo.lastHelpTime[ally] = undefined;
	helpInfo.lastHelpRequest[ally] = undefined;
	helpInfo.helpTimeout[ally] = undefined;
	helpInfo.location[ally].x = undefined;
	helpInfo.location[ally].y = undefined;
	helpInfo.lastHelpPlayer = undefined;

	stopDefendingLocation();
}

function attemptToHelp(player, x, y)
{
	var helpingSelf = (player === me);

	if (!defined(player))
	{
		debugMessage("attemptToHelp. Undefined player parameter.");
		return false;
	}

	if (!defined(x) || !defined(y))
	{
		debugMessage("attemptToHelp. Undefined x or y coordinate.");
		return false;
	}

	//if not helping any other ally or it's me who needs help
	if (helpingSelf || !helpingAlly() || (helpInfo.lastHelpPlayer === player))
	{
		if (!haveHelpers())
		{
			chat(ALL_PLAYERS, CHAT_RESPONSE.noHelp);
		}
	}
	else if (defined(helpInfo.lastHelpPlayer))
	{
		chat(ALL_PLAYERS, "Helping " + getPlayerName(helpInfo.lastHelpPlayer) + " already");
	}

	return false;
}

function helpPlayer(player, x, y)
{
	var timeTravel = 0;
	var attackers = enumGroup(groups.attackers);

	if (!defined(player))
	{
		debugMessage("attemptToHelp. Undefined player parameter.");
		return false;
	}

	if (!defined(x) || !defined(y))
	{
		debugMessage("attemptToHelp. Undefined x or y coordinate.");
		return false;
	}

	//Calculate travel time, assume ~ 150 tiles in 4 minutes
	if (attackers.length === 0)
	{
		timeTravel = Math.floor(distBetweenTwoPoints(BASE.x, BASE.y, x, y) * 1.7 * 1000);
	}
	else
	{
		var avg = groupCoordinateAverage(groups.attackers);
		timeTravel = Math.floor(distBetweenTwoPoints(avg.x, avg.y, x, y) * 1.7 * 1000);
	}

	helpInfo.lastHelpTime[player] = gameTime;
	helpInfo.helpTimeout[player] = gameTime + BASE_DEFEND_DURATION + timeTravel;
	helpInfo.lastHelpPlayer = player;
	helpInfo.location[player].x = x;
	helpInfo.location[player].y = y;

	defendLocation(x, y, helpInfo.helpTimeout[player], (player === me));
}

//FIXME: This is too simple and easily wrong.
//Do we have any units we can send to help ally?
function haveHelpers()
{
	return (enumGroup(groups.attackers).length > 0);
}

function helpingAlly()
{
	return defined(helpInfo.lastHelpPlayer);
}

function canStopHelpingAlly()
{
	if (!helpingAlly())
	{
		return true;
	}

	// Were helping long enough or someone's backstabbing
	if (!friendlyPlayer(helpInfo.lastHelpPlayer))
	{
		return true;
	}

	if (helpInfo.lastHelpTime < gameTime)
	{
		return true;
	}

	return false;
}

function requestHelp(x, y)
{
	var ally = helpInfo.lastHelpPlayer;

	if (!defined(ally))
	{
		return false;
	}

	// Don't do this too frequently
	if (helpInfo.lastHelpRequest[ally] + HELP_REQUEST_INTERVAL > gameTime)
	{
		return false;
	}

	// Remember when we requested help last time
	helpInfo.lastHelpRequest[ally] = gameTime;

	chat(ALLIES, REQUESTS.help);
}

function helpMain()
{
	// Check if we were helping long enough
	if (helpingAlly() && canStopHelpingAlly())
	{
		stopHelpingAlly();
	}

	if (defendingLocation() && defendLocationTimeout())
	{
		stopDefendingLocation();
	}
}
