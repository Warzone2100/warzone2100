
////////////////////////////////////////////////////////////////////////////////
// Transporter management.
////////////////////////////////////////////////////////////////////////////////

/**
 * ## camIsTransporter(gameObject)
 *
 * Determine if the object is a transporter.
 * @param {Object} gameObject
 * @returns {boolean}
 */
function camIsTransporter(gameObject)
{
	if (!camDef(gameObject) || !gameObject)
	{
		return false;
	}

	if (gameObject.type !== DROID)
	{
		camTrace("Attempted to check if a non-droid object is a transporter.");
		return false;
	}

	return gameObject.droidType === DROID_SUPERTRANSPORTER;
}

/**
 * ## camSetupTransporter(placeX, placeY, exitX, exitY)
 *
 * A convenient function for placing the standard campaign transport for loading in pre-away missions.
 * The exit point for the transport is set up as well.
 * @param {number} placeX
 * @param {number} placeY
 * @param {number} exitX
 * @param {number} exitY
 * @returns {void}
 */
function camSetupTransporter(placeX, placeY, exitX, exitY)
{
	addDroid(CAM_HUMAN_PLAYER, placeX, placeY, "Transport", "TransporterBody", "V-Tol", "", "", "MG3-VTOL");
	setTransporterExit(exitX, exitY, CAM_HUMAN_PLAYER);
}

/**
 * ## camRemoveEnemyTransporterBlip()
 *
 * Removes the last blip that an enemy transporter left behind, if any.
 * @returns {void}
 */
function camRemoveEnemyTransporterBlip()
{
	if (camDef(__camTransporterMessage))
	{
		hackRemoveMessage(__camTransporterMessage, PROX_MSG, CAM_HUMAN_PLAYER);
		__camTransporterMessage = undefined;
	}
}

//////////// privates

// returns true if transporter was launched,
// false if transporter is already on map,
// throws error if queue is empty.
function __camDispatchTransporterUnsafe()
{
	if (__camTransporterQueue.length === 0)
	{
		camDebug("Transporter queue empty!");
		return false;
	}
	const OFFSET = 1; //Increaze LZ "no go" zone area a bit
	var args = __camTransporterQueue[0];
	var player = args.player;
	var pos = args.position;
	var list = args.list;
	var data = args.data;
	if (camDef(__camIncomingTransports[player]))
	{
		camTrace("Transporter already on map for player", player + ", delaying.");
		return false;
	}
	__camTransporterQueue.shift(); // what could possibly go wrong?
	if (!camDef(__camPlayerTransports[player]))
	{
		camTrace("Creating a transporter for player", player);
		__camPlayerTransports[player] = addDroid(player, -1, -1,
		                                         "Transporter",
		                                         "TransporterBody",
		                                         "V-Tol", "", "",
		                                         "MG3-VTOL");
	}
	var trans = __camPlayerTransports[player];
	var droids = [];
	for (let i = 0, l = list.length; i < l; ++i)
	{
		var template = list[i];
		var prop = __camChangePropulsionOnDiff(template.prop);
		var droid = addDroid(player, -1, -1, "Reinforcement", template.body, prop, "", "", template.weap);
		droids.push(droid);
		addDroidToTransporter(trans, droid);
	}
	__camIncomingTransports[player] = {
		droids: droids,
		message: args.data.message,
		order: args.order,
		data: args.order_data,
	};
	camTrace("Incoming transport with", droids.length,
	         "droids for player", player +
	         ", queued transports", __camTransporterQueue.length);

	setNoGoArea(pos.x - OFFSET, pos.y - OFFSET, pos.x + OFFSET, pos.y + OFFSET, player);

	//Delete previous enemy reinforcement transport blip
	if (player !== CAM_HUMAN_PLAYER)
	{
		camRemoveEnemyTransporterBlip();
	}

	if (player !== CAM_HUMAN_PLAYER)
	{
		playSound("pcv381.ogg"); //Enemy transport detected.
	}

	setTransporterExit(data.exit.x, data.exit.y, player);
	// will guess which transporter to start, automagically
	startTransporterEntry(data.entry.x, data.entry.y, player);
	return true;
}

function __camDispatchTransporterSafe(player, position, list, data)
{
	if (!profile("__camDispatchTransporterUnsafe"))
	{
		queue("__camDispatchTransporterSafe", camSecondsToMilliseconds(1));
	}
}

function __camLandTransporter(player, pos)
{
	var ti = __camIncomingTransports[player];
	if (!camDef(ti))
	{
		camDebug("Unhandled transporter for player", player);
		return;
	}
	if (camDef(ti.message))
	{
		__camTransporterMessage = ti.message;
		hackAddMessage(ti.message, PROX_MSG, CAM_HUMAN_PLAYER, false);
	}
	else
	{
		__camTransporterMessage = undefined;
	}
	camTrace("Landing transport for player", player);
	playSound("pcv395.ogg", pos.x, pos.y, 0); //Incoming enemy transport.
	camManageGroup(camMakeGroup(ti.droids), ti.order, ti.data);
}

function __camRemoveIncomingTransporter(player)
{
	// allow the next transporter to enter
	if (camDef(__camIncomingTransports[player]))
	{
		delete __camIncomingTransports[player];
	}
}
