//Cache/throttle/other similar stuff goes here.

//callFuncWithArgs(FUNCTION_NAME, ARRAY_OF_PARAMETERS);
function callFuncWithArgs(func, parameters)
{
	return func.apply(this, parameters);
}

//cacheThis(FUNCTION_NAME, [FUNCTION_PARAMETERS], [CACHED_NAME], [TIME])
//Pass in Infinity for time to never recalculate it again.
function cacheThis(func, funcParameters, cachedItem, time)
{
	const REFRESH_TIME = isDefined(time) ? time : 15000;
	if (!isDefined(cachedItem))
	{
		cachedItem = 0;
	}
	if (!isDefined(funcParameters))
	{
		funcParameters = [];
	}

	if ((time === Infinity) && isDefined(debugGetCallerFuncObject().cachedValues))
	{
		return debugGetCallerFuncObject().cachedValues[cachedItem];
	}

	if (!isDefined(debugGetCallerFuncObject().cachedTimes))
	{
		debugGetCallerFuncObject().cachedTimes = {};
		debugGetCallerFuncObject().cachedValues = {};
	}

	var t = debugGetCallerFuncObject().cachedTimes[cachedItem];
	var obj = debugGetCallerFuncObject().cachedValues[cachedItem];
	var def = isDefined(obj);
	if (!def ||
		(isDefined(obj.typeInfo) && (getObject(obj.typeInfo, obj.playerInfo, obj.idInfo) === null)) ||
		((gameTime - t) >= REFRESH_TIME))
	{
		debugGetCallerFuncObject().cachedValues[cachedItem] = callFuncWithArgs(func, funcParameters);
		debugGetCallerFuncObject().cachedTimes[cachedItem] = gameTime;
	}

	return debugGetCallerFuncObject().cachedValues[cachedItem];
}

//A simple throttle function to make sure something is not executed too much.
function stopExecution(throttleThis, time)
{
	if (!isDefined(time))
	{
		time = 2000;
	}

	if (!isDefined(debugGetCallerFuncObject().throttleTimes))
	{
		debugGetCallerFuncObject().throttleTimes = {};
	}

	if (!isDefined(debugGetCallerFuncObject().throttleTimes[throttleThis]))
	{
		debugGetCallerFuncObject().throttleTimes[throttleThis] = gameTime;
		return false;
	}

	if (gameTime - debugGetCallerFuncObject().throttleTimes[throttleThis] < time)
	{
		return true;
	}

	debugGetCallerFuncObject().throttleTimes[throttleThis] = gameTime;
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
