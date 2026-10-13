// The last run time of each throttled action, by name (a var global, so that a saved game keeps it).
var throttleTimes = {};

//A simple throttle function to make sure something is not executed too much.
function ThrottleThis(throttleThis, time)
{
	if (!defined(time))
	{
		time = 2000;
	}

	if (!defined(throttleTimes[throttleThis]))
	{
		throttleTimes[throttleThis] = gameTime;
		return false;
	}

	if (gameTime - throttleTimes[throttleThis] < time)
	{
		return true;
	}

	throttleTimes[throttleThis] = gameTime;

	return false;
}
