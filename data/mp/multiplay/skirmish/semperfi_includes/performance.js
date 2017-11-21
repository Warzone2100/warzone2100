
//A simple throttle function to make sure something is not executed too much.
function stopExecution(throttleThis, time)
{
     if (!defined(time))
     {
          time = 2000;
     }

     if (!defined(arguments.callee.caller.throttleTimes))
     {
          arguments.callee.caller.throttleTimes = {};
     }

	if (!defined(arguments.callee.caller.throttleTimes[throttleThis]))
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
