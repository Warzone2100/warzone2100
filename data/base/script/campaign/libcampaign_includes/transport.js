
////////////////////////////////////////////////////////////////////////////////
// Transporter management.
////////////////////////////////////////////////////////////////////////////////

//;; ## camIsTransporter(gameObject)
//;;
//;; Determine if the object is a transporter.
//;;
//;; @param {Object} gameObject
//;; @returns {boolean}
//;;
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

//;; ## camSetupTransporter(placeX, placeY, exitX, exitY)
//;;
//;; A convenient function for placing the standard campaign transport for loading in pre-away missions.
//;; The exit point for the transport is set up as well.
//;;
//;; @param {number} placeX
//;; @param {number} placeY
//;; @param {number} exitX
//;; @param {number} exitY
//;; @returns {void}
//;;
function camSetupTransporter(placeX, placeY, exitX, exitY)
{
	addDroid(CAM_HUMAN_PLAYER, placeX, placeY, cam_trComps.name, cam_trComps.body, cam_trComps.propulsion, "", "", cam_trComps.weapon);
	setTransporterExit(exitX, exitY, CAM_HUMAN_PLAYER);
}

//;; ## camRemoveEnemyTransporterBlip()
//;;
//;; Removes the last blip that an enemy transporter left behind, if any.
//;;
//;; @returns {void}
//;;
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
	const __OFFSET = 1; //Increaze LZ "no go" zone area a bit
	const args = __camTransporterQueue[0];
	const __PLAYER = args.player;
	const pos = args.position;
	const list = args.list;
	const data = args.data;
	if (camDef(__camIncomingTransports[__PLAYER]))
	{
		camTrace("Transporter already on map for player", __PLAYER + ", delaying.");
		return false;
	}
	__camTransporterQueue.shift(); // what could possibly go wrong?
	if (!camDef(__camPlayerTransports[__PLAYER]))
	{
		camTrace("Creating a transporter for player", __PLAYER);
		__camPlayerTransports[__PLAYER] = addDroid(__PLAYER, -1, -1,
		                                         cam_trComps.name,
		                                         cam_trComps.body,
		                                         cam_trComps.propulsion, "", "",
		                                         cam_trComps.weapon);
	}
	const transporter = __camPlayerTransports[__PLAYER];
	const droids = [];
	for (let i = 0, l = list.length; i < l; ++i)
	{
		const template = list[i];
		const __PROP = __camChangePropulsion(template.prop, __PLAYER);
		const droid = addDroid(__PLAYER, -1, -1, "Reinforcement", template.body, __PROP, "", "", template.weap);
		droids.push(droid);
		addDroidToTransporter(transporter, droid);
	}
	__camIncomingTransports[__PLAYER] = {
		droids: droids,
		message: args.data.message,
		order: args.order,
		data: args.order_data,
	};
	camTrace("Incoming transport with", droids.length,
	         "droids for player", __PLAYER +
	         ", queued transports", __camTransporterQueue.length);

	setNoGoArea(pos.x - __OFFSET, pos.y - __OFFSET, pos.x + __OFFSET, pos.y + __OFFSET, __PLAYER);

	//Delete previous enemy reinforcement transport blip
	if (__PLAYER !== CAM_HUMAN_PLAYER)
	{
		camRemoveEnemyTransporterBlip();
	}

	if (__PLAYER !== CAM_HUMAN_PLAYER)
	{
		playSound(cam_sounds.transport.enemyTransportDetected);
	}

	setTransporterExit(data.exit.x, data.exit.y, __PLAYER);
	// will guess which transporter to start, automagically
	startTransporterEntry(data.entry.x, data.entry.y, __PLAYER);
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
	const ti = __camIncomingTransports[player];
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
	playSound(cam_sounds.transport.incomingEnemyTransport, pos.x, pos.y, 0);
	camManageGroup(camMakeGroup(ti.droids), ti.order, ti.data);
	if (player !== CAM_HUMAN_PLAYER)
	{
		for (let i = 0, len = ti.droids.length; i < len; ++i)
		{
			camSetDroidExperience(ti.droids[i]);
		}
	}
}

function __camRemoveIncomingTransporter(player)
{
	// allow the next transporter to enter
	if (camDef(__camIncomingTransports[player]))
	{
		delete __camIncomingTransports[player];
	}
}
