
/*
 * This file includes generic functions for improving CPU intensity,
 * like caching and throttling mechanisms.
 *
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

// The throttle and cache state of each calling function, by the function's name.
// Global variables, so that a saved game keeps them (properties set on a function object start over on load).
// Every caller of throttled() and cached() must therefore be a named function with a name of its own.
// NOTE: an anonymous caller falls back to state kept on the function object, which a saved game loses.
_global.throttleTimes = {};
_global.cachedTimes = {};
_global.cachedValues = {};

function callerStateTable(table, caller, prop)
{
	if (caller.name === "")
	{
		if (!defined(caller[prop]))
		{
			caller[prop] = {};
		}

		return caller[prop];
	}

	if (!defined(table[caller.name]))
	{
		table[caller.name] = {};
	}

	return table[caller.name];
}

// to make sure some function is not called more often than every 2 seconds,
// put something like that at start the of its body:
//		if (throttled(2000)) return;
// if your function requires several throttling cases, you can use the optional
// notes value:
//		if (throttled(60000, player)) return;
// NOTE: the calling function must be named (see throttleTimes above), and
// it won't work if the function repeatedly dies and gets created again, ex.
// a function defined inside forEach((...) => {...}) can't be throttled
_global.throttled = function(interval, notes) {
	const times = callerStateTable(throttleTimes, debugGetCallerFuncObject(), "throttleTimes");

	if (!defined(times[notes]))
	{
		times[notes] = gameTime;
		return false;
	}

	if (gameTime - times[notes] < interval)
	{
		return true;
	}

	times[notes] = gameTime;

	return false;
};

// to cache a function's output value and make sure it's not re-calculated too often,
// use the following trick:
//		function calculateValue(params) {
//			function uncached(params) {
//				// do heavy calculations
//			}
//			return cached(uncached, 10000);
//		}
// add necessary notes to the cached() call if necessary, similar to how you do it
// for throttled().
// NOTE: the calling function must be named (see throttleTimes above), and
// it won't work if the function repeatedly dies and gets created again, ex.
// a function defined inside forEach((...) => {...}) can't have caching inside
_global.cached = function(whatToCall, interval, notes) {
	const caller = debugGetCallerFuncObject();
	const times = callerStateTable(cachedTimes, caller, "cachedTimes");
	const values = callerStateTable(cachedValues, caller, "cachedValues");
	const t = times[notes];

	if (!defined(t) || gameTime - t >= interval)
	{
		values[notes] = whatToCall();
		times[notes] = gameTime;
	}

	return values[notes];
};

// if you actually want your script to send debug messages, consider using this function.
// it will only output each message only once, so your debug log will be readable.
_global.niceDebug = function niceDebug() {
	const msg = me + ": " + Array.prototype.join.call(arguments, " ");

	if (throttled(Infinity, msg))
	{
		return;
	}

	debug(msg);
};

// The position of each function series, by id (a global variable, so that a saved game keeps it)
_global.functionSeriesLast = {};

// use this if you want to split a certain void function into parts.
// example:
//		function first()  { return random(2); }
//		function second() { return random(2); }
//		function third()  { return random(2); }
//		function doManyThings() {
//			if (functionSeries("doStuff", [ first, second, third ]))
//				queue("doManyThings");
//		}
// NOTE: in this example functions of the list don't need to be global, but
// doManyThings does, otherwise the queue call will fail.
_global.functionSeries = function(id, list) {
	if (!defined(functionSeriesLast[id]))
	{
		functionSeriesLast[id] = 0;
	}
	else
	{
		++functionSeriesLast[id];
	}

	if (functionSeriesLast[id] >= list.length)
	{
		functionSeriesLast[id] = 0;
		return false; // all functions in the list were called, none succeeded
	}

	if (!list[functionSeriesLast[id]]())
	{
		return true; // none of the function succeeded yet, telling to call us again next time
	}

	functionSeriesLast[id] = undefined;

	return false; // one of the functions succeeded, no need to call us anymore
};

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
