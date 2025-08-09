
function eventChat(from, to, message)
{
	if (from === me)
	{
		if (message === "debug on")
		{
			debugMode = true;
		}
		else if (message === "debug off")
		{
			debugMode = false;
		}
		else if (isNexusPersonality(message))
		{
			initPersonalityData(message);
		}

		return;
	}

	if (!allianceExistsBetween(me, from))
	{
		return;
	}

	processCommand(message, from, false);
}

function processCommand(message, sender, blipMessage)
{
	if (message === REQUESTS.help)
	{
		if (haveBeacon(sender))
		{
			if (attemptToHelp(sender,
				helpInfo.location[sender].x,
				helpInfo.location[sender].y))
			{
				chat(ALL_PLAYERS, CHAT_RESPONSE.confirm);
			}
		}
		else
		{
			//Ask player to drop a beacon so we would know where to help
			chat(sender, CHAT_RESPONSE.beacon);
		}
	}
	/* Someone requested a beacon from us -
	* did we request help and our beacon timed out??
	*/
	else if (message === REQUESTS.beacon)
	{
		// If our base is really in trouble drop a beacon for the requester again
		if (baseInTrouble())
		{
			addBeacon(BASE.x, BASE.y, me, CHAT_RESPONSE.help);
		}
	}
	else if (message === REQUESTS.safety)
	{
		// Make sure we were helping them
		if (helpingAlly() && helpInfo.lastHelpPlayer === sender)
		{
			stopHelpingAlly();
			chat(ALL_PLAYERS, CHAT_RESPONSE.affirmative);
		}
		else if (numEnemiesInBase() > 0)
		{
			//if we are in trouble re-request help
			requestHelp(BASE.x, BASE.y);
		}
	}
	else if (isNexusPersonality(message))
	{
		initPersonalityData(message);
	}
	else
	{
		debugMessage("processCommand. Unknown message: " + message);
	}
}
