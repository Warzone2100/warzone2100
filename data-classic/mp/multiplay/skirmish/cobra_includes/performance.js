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

     if ((time === Infinity) && isDefined(arguments.callee.caller.cachedValues))
     {
          return arguments.callee.caller.cachedValues[cachedItem];
     }

     if (!isDefined(arguments.callee.caller.cachedTimes))
     {
		arguments.callee.caller.cachedTimes = {};
		arguments.callee.caller.cachedValues = {};
	}

	var t = arguments.callee.caller.cachedTimes[cachedItem];
     var obj = arguments.callee.caller.cachedValues[cachedItem];
     var def = isDefined(obj);
	if (!def ||
          (isDefined(obj.typeInfo) && (getObject(obj.typeInfo, obj.playerInfo, obj.idInfo) === null)) ||
          ((gameTime - t) >= REFRESH_TIME))
     {
		arguments.callee.caller.cachedValues[cachedItem] = callFuncWithArgs(func, funcParameters);
		arguments.callee.caller.cachedTimes[cachedItem] = gameTime;
	}

     return arguments.callee.caller.cachedValues[cachedItem];
}

//A simple throttle function to make sure something is not executed too much.
function stopExecution(throttleThis, time)
{
     if (!isDefined(time))
     {
          time = 2000;
     }

     if (!isDefined(arguments.callee.caller.throttleTimes))
     {
          arguments.callee.caller.throttleTimes = {};
     }

	if (!isDefined(arguments.callee.caller.throttleTimes[throttleThis]))
     {
		arguments.callee.caller.throttleTimes[throttleThis] = gameTime;
		return false;
	}

	if (gameTime - arguments.callee.caller.throttleTimes[throttleThis] < time)
     {
          return true;
     }

	arguments.callee.caller.throttleTimes[throttleThis] = gameTime;
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
