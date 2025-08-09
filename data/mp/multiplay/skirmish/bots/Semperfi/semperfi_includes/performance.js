
//A simple throttle function to make sure something is not executed too much.
function ThrottleThis(throttleThis, time)
{
	if (!defined(time))
	{
		time = 2000;
	}

	if (!defined(debugGetCallerFuncObject().throttleTimes))
	{
		debugGetCallerFuncObject().throttleTimes = {};
	}

	if (!defined(debugGetCallerFuncObject().throttleTimes[throttleThis]))
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
