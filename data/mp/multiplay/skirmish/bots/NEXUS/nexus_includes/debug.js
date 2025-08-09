
function debugMessage(message)
{
	if (debugMode && defined(message))
	{
		dump(gameTime + " : " + message);
	}
}

function debugObject(object, message)
{
	if (debugMode && object && defined(message))
	{
		dump(gameTime + " [" + object.name + " id=" + object.id + "] > " + message);
	}
}

function consoleMessage(message, logIt, object)
{
	if (!debugMode)
	{
		return;
	}

	if (!defined(message))
	{
		debugMessage("consoleMessage. Undefined message.");
		return;
	}

	if (!defined(logIt))
	{
		logIt = false;
	}

	console(gameTime + " [" + message + "]");

	if (logIt)
	{
		if (defined(object))
		{
			debugObject(object, message);
		}
		else
		{
			debugMessage(message);
		}
	}
}
