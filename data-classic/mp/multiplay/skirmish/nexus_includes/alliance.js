
function allianceTimeInit()
{
	if (!defined(allianceTime))
	{
		allianceTime = [];

		for (var i = 0; i < maxPlayers; ++i)
		{
			allianceTime.push(0);
		}
	}
}

function validAllianceTimeReached()
{
	return (gameTime > minutesToMilliseconds(3.5));
}

function eventAllianceOffer(from, to)
{
	if (!validAllianceTimeReached() || tooManyAlliances())
	{
		return;
	}

	if (to === me && isPlayerAlive(from) && isPlayerAlive(me))
	{
		if (random(28) === 0)
		{
			setAlliance(from, to, true);
		}
	}
}

function eventAllianceAccepted(from, to)
{
	if (from === me || to === me)
	{
		allianceTimeInit();

		if (from !== me)
		{
			allianceTime[from] = gameTime;
		}
		else
		{
			allianceTime[to] = gameTime;
		}
	}
}

function eventAllianceBroken(from, to)
{
	if (from === me || to === me)
	{
		allianceTimeInit();

		if (from !== me)
		{
			allianceTime[from] = gameTime;
		}
		else
		{
			allianceTime[to] = gameTime;
		}
	}
}

function tooManyAlliances()
{
	return (getAllyPlayers().length > (Math.floor(maxPlayers / 2) - 1));
}

//Specify any true parameter to only return the ones that are still alive.
function getAllyPlayers(alive)
{
	var allies = [];

	if (!defined(alive))
	{
		alive = false;
	}

	for (var i = 0; i < maxPlayers; ++i)
	{
		if (i !== me && allianceExistsBetween(me, i))
		{
			if (alive && !isPlayerAlive(i))
			{
				continue; //Nope. They are dead.
			}

			allies.push(i);
		}
	}

	return allies;
}

function tryBreakingAlliance()
{
	var allies = getAllyPlayers(true);

	for (var i = 0, len = allies.length; i < len; ++i)
	{
		var ally = allies[i];
		var breakIt = false;
		//Break an alliance at random if we are allies for some amount of time.
		if (random(30) === 0 && (gameTime - allianceTime[ally]) > minutesToMilliseconds(10))
		{
			breakIt = true;
		}
		//FIXME: These original rules are really terrible and often times alliances
		//will fluctuate rapidly with the below rules.
		// rules for breaking alliances with humans.
		// built within my base
		if (!breakIt && numWeapObjectsInRange(BASE.x, BASE.y, ally, baseLimits.radius, true).structures > 1)
		{
			breakIt = true;
		}
		// you've got lots of units in my area.
		if (!breakIt && numDroidsInBase(ally) > 3)
		{
			breakIt = true;
		}
		// you've wiped out one of my allies ??. --TBD
		if (breakIt)
		{
			setAlliance(me, ally, false);
		}
	}
}

function tryFormingAlliances()
{
	if (!validAllianceTimeReached() || tooManyAlliances())
	{
		return;
	}

	for (var i = 0; i < maxPlayers; ++i)
	{
		var time = allianceTime[i];
		var tooEarly = (gameTime - time < minutesToMilliseconds(12));

		if (i !== me && !allianceExistsBetween(me, i) && (time === 0 || tooEarly === false))
		{
			if (isPlayerAlive(i) && random(20) === 0)
			{
				sendAllianceRequest(i);
			}
		}
	}
}

//See if some location resides in an ally base.
function allyBaseAtLoc(player, x, y)
{
	if (!defined(player))
	{
		debugMessage("allyBaseAtLoc. Undefined player parameter.");
		return false;
	}

	if (player === me || !allianceExistsBetween(me, player))
	{
		debugMessage("allyBaseAtLoc. This should not be used for 'me' or enemies.");
		return false;
	}

	if (!defined(x) || !defined(y))
	{
		debugMessage("allyBaseAtLoc. Undefined x or y coordinate.");
		return false;
	}

	return insideBase(x, y, baseDetails(player));
}

//Spread out over 2 ticks
function allianceMain()
{
	if (alliancesType === ALLIANCES && isPlayerAlive(me))
	{
		tryFormingAlliances();
		queue("tryBreakingAlliance", TICK);
	}
}
