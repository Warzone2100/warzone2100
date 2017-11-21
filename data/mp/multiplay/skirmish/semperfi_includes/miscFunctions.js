//Functions that have no other good place to be.

// --- utility functions
function log(message)
{
	dump(gameTime + " : " + message);
}

function logObj(obj, message)
{
	dump(gameTime + " [" + obj.name + " id=" + obj.id + "] > " + message);
}

// Returns true if something is defined
function defined(data)
{
	return typeof(data) !== "undefined";
}

//Return a number in the range of 0 to (max - 1).
function random(max)
{
	return (max <= 0) ? 0 : Math.floor(Math.random() * max);
}

//Return our real power level
function getRealPower()
{
	return playerPower(me) - queuedPower(me);
}

function sortByDistToBase(obj1, obj2)
{
	var dist1 = distBetweenTwoPoints(BASE.x, BASE.y, obj1.x, obj1.y);
	var dist2 = distBetweenTwoPoints(BASE.x, BASE.y, obj2.x, obj2.y);
	return (dist1 - dist2);
}

//Used for deciding if a truck will capture oil.
function isUnsafeEnemyObject(obj)
{
	return ((obj.type === DROID)
		|| (obj.type === STRUCTURE
		&& obj.stattype === DEFENSE
		&& obj.status === BUILT)
	);
}
