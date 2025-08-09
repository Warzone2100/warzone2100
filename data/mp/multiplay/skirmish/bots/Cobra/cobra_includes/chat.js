
//A way to control chat messages sent to Cobra AI.
function sendChatMessage(msg, receiver)
{
	if (!isDefined(msg))
	{
		return;
	}
	if (!isDefined(receiver))
	{
		receiver = ALLIES;
	}

	if ((lastMsg !== msg) || (gameTime >= lastMsgThrottle + 15000))
	{
		lastMsg = msg;
		lastMsgThrottle = gameTime;

		if (receiver === ALLIES || receiver === ENEMIES)
		{
			let players = playerAlliance(receiver === ALLIES);
			for (let i = 0, len = players.length; i < len; ++i)
			{
				if (msg === "need power" && !playerData[players[i]].isAI)
				{
					continue; //don't spam humans with power requests.
				}
				chat(players[i], msg);
			}
		}
		else if (receiver === ALL_PLAYERS)
		{
			chat(ALL_PLAYERS, msg);
		}
		else
		{
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
	if (message === "cobra strategy")
	{
		sendChatMessage("I am using \"" + myPersonality() + "\" tactics.", ALLIES);
	}
	else if (message === "cobra strat list")
	{
		sendChatMessage("My strategies are: AC, AR, AB, AM, AA", ALLIES);
	}
	else if (message === "AC" ||
		message === "AR" ||
		message === "AB" ||
		message === "AM" ||
		message === "AA")
	{
		if (personality !== message)
		{
			choosePersonality(message);
		}
	}
	else if (message === "toggle cyborg")
	{
		turnOffCyborgs = !turnOffCyborgs;
		if (from !== me)
		{
			sendChatMessage("Cyborgs " + (turnOffCyborgs ? "off" : "on"), from);
		}
		sendChatMessage("Cyborgs " + (turnOffCyborgs ? "off" : "on"), me);
	}
	else if (message === "stats")
	{
		if (from !== me)
		{
			sendChatMessage("Most harmful player: " + getMostHarmfulPlayer(), from);
		}
		sendChatMessage("Most harmful player: " + getMostHarmfulPlayer(), me);
	}
	else if (message === "toggle hover" && !checkIfSeaMap())
	{
		forceHover = !forceHover;
		if (from !== me)
		{
			sendChatMessage("Forced hover " + (forceHover ? "on" : "off"), from);
		}
		sendChatMessage("Forced hover " + (forceHover ? "on" : "off"), me);
	}
	else if (message === "oil level")
	{
		if (from !== me)
		{
			sendChatMessage("Map oil count is " + mapOilLevel(), from);
		}
		sendChatMessage("Map oil count is " + mapOilLevel(), me);
	}
	else if (message === "toggle arti")
	{
		useArti = !useArti;
		if (from !== me)
		{
			sendChatMessage("Artillery " + (useArti ? "on" : "off"), from);
		}
		sendChatMessage("Artillery " + (useArti ? "on" : "off"), me);
	}
	else if (message === "toggle vtol")
	{
		useVtol = !useVtol;
		if (from !== me)
		{
			sendChatMessage("VTOLs " + (useVtol ? "on" : "off"), from);
		}
		sendChatMessage("VTOLs " + (useVtol ? "on" : "off"), me);
	}
	else if (message === "resG" || message === "resO" || message === "resD" || message === "resA")
	{
		let pth = "";
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

		subPersonalities[personality].resPath = pth;
		sendChatMessage("research path changed to: " + pth, ALLIES);
	}
	else if (message === "toggle beacon")
	{
		beacon.disabled = !beacon.disabled;
		if (from !== me)
		{
			sendChatMessage("Beacon behavior: " + (beacon.disabled ? "off" : "on"), from);
		}
		sendChatMessage("Beacon behavior: " + (beacon.disabled ? "off" : "on"), me);
	}


	//Do not execute these statements if from is me or enemy.
	if (to === from)
	{
		return;
	}

	if (message === "need truck")
	{
		if (donateFromGroup(from, "TRUCK"))
		{
			sendChatMessage("I give you some of my construction units", from);
		}
		else
		{
			sendChatMessage("I have no construction units to give you", from);
		}
	}
	else if (message === "need power")
	{
		if (getRealPower(me) > 100)
		{
			donatePower(playerPower(me) / 5, from);
			if (!playerData[from].isAI)
			{
				sendChatMessage("Here is some power", from);
			}
		}
		else
		{
			if (!playerData[from].isAI)
			{
				sendChatMessage("I do not have enough power to give you any", from);
			}
		}
	}
	else if (message === "need tank")
	{
		if (donateFromGroup(from, "ATTACK"))
		{
			sendChatMessage("Here are some tanks", from);
		}
		else
		{
			sendChatMessage("I have no spare tanks", from);
		}
	}
	else if (message === "need cyborg")
	{
		if (donateFromGroup(from, "CYBORG"))
		{
			sendChatMessage("Here are some cyborgs", from);
		}
		else
		{
			sendChatMessage("I have no spare cyborgs", from);
		}
	}
	else if (message === "need vtol")
	{
		if (donateFromGroup(from, "VTOL"))
		{
			sendChatMessage("Here are some VTOLs", from);
		}
		else
		{
			sendChatMessage("I have no spare VTOLs", from);
		}
	}

	//Here be commands that do something to a specific enemy.
	const REAL_MSG = message.slice(0, -1);
	if (REAL_MSG === "target")
	{
		let num = message.slice(-1);
		if (!allianceExistsBetween(num, me) && (num !== me))
		{
			if (targetPlayer(num))
			{
				sendChatMessage("Target set to player " + num, from);
			}
			else
			{
				sendChatMessage("Sorry, target not changed to player " + num + " (they may be my target already)", from);
			}
		}
		else
		{
			sendChatMessage("Can not attack myself or an ally which is player " + num, from);
		}
	}
}
