
//A way to control chat messages sent to Cobra AI.
function sendChatMessage(msg, receiver)
{
	if (isDefined(msg))
	{
		if (!isDefined(receiver))
		{
			receiver = ALLIES;
		}
		if (lastMsg !== msg)
		{
			lastMsg = msg;
			chat(receiver, msg);
		}
	}
}

function eventChat(from, to, message)
{
	if ((to !== me) || !allianceExistsBetween(from, to))
	{
		return;
	}

	//Here are all chat messages that can be executed by itself.
	if (message === "AC" ||
		message === "AR" ||
		message === "AB" ||
		message === "AM" ||
		message === "AA" ||
		message === "AL")
	{
		if (personality !== message)
		{
			choosePersonality(message);
		}
	}
	else if (message === "toggle cyborg")
	{
		turnOffCyborgs = !turnOffCyborgs;
	}
	else if (message === "stats")
	{
		sendChatMessage(MostHarmfulPlayer(), to);
	}
	else if (message === "toggle hover")
	{
		forceHover = !forceHover;
	}
	else if (message === "oil level")
	{
		sendChatMessage("Map oil count is: " + mapOilLevel(), ALLIES);
	}
	else if (message === "toggle arti")
	{
		useArti = !useArti;
	}
	else if (message === "toggle vtol")
	{
		useVtol = !useVtol;
	}
	else if (message === "resG" || message === "resO" || message === "resD" || message === "resA")
	{
		var pth = "";
		if (message === "resG")
		{
			pth = "generic";
		}
		else if (message === "resO")
		{
			pth = "offensive";
		}
		else if (message === "resD")
		{
			pth = "defensive";
		}
		else if (message === "resA")
		{
			pth = "air";
		}

		prevResPath = subPersonalities[personality].resPath;
		subPersonalities[personality].resPath = pth;
		sendChatMessage("research path changed to: " + pth, ALLIES);
	}


	//Do not execute these statements if from is me or enemy.
	if (to === from)
	{
		return;
	}

	if (message === "need truck")
	{
		var droids = enumGroup(constructGroup).filter(function(dr) {
			return (dr.health > 90);
		});
		var cacheDroids = droids.length;
		if (cacheDroids > 2)
		{
			donateObject(droids[random(cacheDroids)], from);
		}
	}
	else if (message === "need power")
	{
		if (playerPower(me) > 50)
		{
			donatePower(playerPower(me) / 2, from);
		}
	}
	else if (message === "need tank")
	{
		donateFromGroup(from, "ATTACK");
	}
	else if (message === "need cyborg")
	{
		donateFromGroup(from, "CYBORG");
	}
	else if (message === "need vtol")
	{
		donateFromGroup(from, "VTOL");
	}

	//Here be commands that do something to a specific enemy.
	const REAL_MSG = message.slice(0, -1);
	if (REAL_MSG === "target")
	{
		var num = message.slice(-1);
		if (!allianceExistsBetween(num, me) && (num !== me))
		{
			targetPlayer(num);
		}
	}
}
