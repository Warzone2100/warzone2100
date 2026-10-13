//Cache/throttle/other similar stuff goes here.

//callFuncWithArgs(FUNCTION_NAME, ARRAY_OF_PARAMETERS);
function callFuncWithArgs(func, parameters)
{
	return func.apply(this, parameters);
}

// The cache and throttle state of each calling function, by the function's name.
// Var globals, so that a saved game keeps them (properties set on a function object start over on load).
// Every caller of cacheThis() and stopExecution() must therefore be a named function with a name of its own.
// NOTE: an anonymous caller falls back to state kept on the function object, which a saved game loses.
var cachedTimes = {};
var cachedValues = {};
var throttleTimes = {};

// The table of the calling function `caller` in `table`, or undefined if there is none yet (create: make one)
function callerTable(table, caller, prop, create)
{
	if (caller.name === "")
	{
		if (create && !isDefined(caller[prop]))
		{
			caller[prop] = {};
		}

		return caller[prop];
	}

	if (create && !isDefined(table[caller.name]))
	{
		table[caller.name] = {};
	}

	return table[caller.name];
}

//cacheThis(FUNCTION_NAME, [FUNCTION_PARAMETERS], [CACHED_NAME], [TIME])
//Pass in Infinity for time to never recalculate it again.
function cacheThis(func, funcParameters, cachedItem, time)
{
	const __refreshTime = isDefined(time) ? time : 15000;
	const _caller = debugGetCallerFuncObject();

	if (!isDefined(cachedItem))
	{
		cachedItem = 0;
	}

	if (!isDefined(funcParameters))
	{
		funcParameters = [];
	}

	if ((time === Infinity) && isDefined(callerTable(cachedValues, _caller, "cachedValues", false)))
	{
		return callerTable(cachedValues, _caller, "cachedValues", false)[cachedItem];
	}

	const _times = callerTable(cachedTimes, _caller, "cachedTimes", true);
	const _values = callerTable(cachedValues, _caller, "cachedValues", true);
	const __t = _times[cachedItem];
	const _obj = _values[cachedItem];
	const __def = isDefined(_obj);

	if (!__def ||
		(isDefined(_obj.typeInfo) && (getObject(_obj.typeInfo, _obj.playerInfo, _obj.idInfo) === null)) ||
		((gameTime - __t) >= __refreshTime))
	{
		_values[cachedItem] = callFuncWithArgs(func, funcParameters);
		_times[cachedItem] = gameTime;
	}

	return _values[cachedItem];
}

//A simple throttle function to make sure something is not executed too much.
function stopExecution(throttleThis, time)
{
	if (!isDefined(time))
	{
		time = 2000;
	}

	const _times = callerTable(throttleTimes, debugGetCallerFuncObject(), "throttleTimes", true);

	if (!isDefined(_times[throttleThis]))
	{
		_times[throttleThis] = gameTime;
		return false;
	}

	if (gameTime - _times[throttleThis] < time)
	{
		return true;
	}

	_times[throttleThis] = gameTime;
	return false;
}

//This way we can pass around DROID/STRUCTURE/FEATURE object information in cacheThis by using getObject()
//in the calling functions since assigning a variable only keeps a snapshot of the object as it was at the time.
function objectInformation(object)
{
	return {
		"typeInfo": object.type,
		"playerInfo": object.player,
		"idInfo": object.id
	};
}
