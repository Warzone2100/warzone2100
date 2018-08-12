
function isPlayerAlive(player)
{
	if (!defined(player))
	{
		debugMessage("isPlayerAlive. Undefined player parameter.");
		return false;
	}

	if ((countStruct(BASE_STRUCTURES.factories, player) +
		countStruct(BASE_STRUCTURES.templateFactories, player) +
		countDroid(DROID_ANY, player)) > 0)
	{
		return true;
	}

	return false;
}

//Return all enemy players that are still alive.
function getAliveEnemyPlayers()
{
	var numEnemies = [];

	for (var i = 0; i < maxPlayers; ++i)
	{
		if (i !== me && !allianceExistsBetween(me, i) && isPlayerAlive(i))
		{
			numEnemies.push(i);
		}
	}

	if (defined(scavengerPlayer) &&
		(countStruct("A0BaBaFactory", scavengerPlayer) +
		countStruct(BASE_STRUCTURES.derricks, scavengerPlayer) +
		countDroid(DROID_ANY, scavengerPlayer)) > 0)
	{
		numEnemies.push(scavengerPlayer);
	}

	return numEnemies;
}

function setPlayerAsTarget(player)
{
	if (!defined(player))
	{
		debugMessage("setPlayerAsTarget. Undefined player parameter.");
		return;
	}

	targetInfo.currentEnemy = player;
}

//Expects an object = {type: DROID/STRUCTURE, player: player, id: id}
function setObjectAsTarget(obj)
{
	if (!defined(obj))
	{
		debugMessage("setObjectAsTarget. Undefined object parameter.");
		return;
	}

	if (!defined(obj.type) || !defined(obj.player) || !defined(obj.id))
	{
		debugMessage("setObjectAsTarget. Insufficient object properties.");
		return;
	}

	targetInfo.what = obj;
	targetInfo.currentEnemy = obj.player;
}

function resetTargetData()
{
	targetInfo = {
		currentEnemy: undefined, //who we prefer to attack
		what: undefined, //{type, player, id}
		vtolGroupWhat: [], //{type, player, id}
	};
}

function chooseEnemy()
{
	var enemies = getAliveEnemyPlayers();

	if (enemies.length === 0)
	{
		resetTargetData();
		return undefined; //no more enemy players are alive.
	}

	setPlayerAsTarget(enemies[random(enemies.length)]);
}

function getCurrentEnemy()
{
	if (!defined(targetInfo.currentEnemy) || !defined(targetInfo.what))
	{
		chooseEnemy();
	}

	//Look if we made an alliance with this player
	if ((defined(targetInfo.currentEnemy) && allianceExistsBetween(me, targetInfo.currentEnemy)) ||
		(defined(targetInfo.what) && allianceExistsBetween(me, targetInfo.what.player)))
	{
		resetTargetData();
		chooseEnemy();
	}

	return targetInfo.currentEnemy;
}
