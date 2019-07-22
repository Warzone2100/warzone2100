
////////////////////////////////////////////////////////////////////////////////
// Reinforcement related stuff.
////////////////////////////////////////////////////////////////////////////////

//;; ## camSendReinforcement(player, position, droids, kind, data)
//;;
//;; Give a single bunch of droids (template list) for a player at
//;; a position label. Kind can be one of:
//;;
//;; * ```CAM_REINFORCE_GROUND``` Reinforcements magically appear
//;; 	on the ground.
//;; * ```CAM_REINFORCE_TRANSPORT``` Reinforcements are unloaded from
//;; 	a transporter.
//;; 	__NOTE:__ the game engine doesn't seem to support two simultaneous
//;; 	incoming transporters for the same player. Avoid this at all costs!
//;; 	The following data fields are required:
//;;   * ```entry``` Transporter entry position.
//;;   * ```exit``` Transporter exit position.
//;;   * ```message``` ```PROX_MSG``` to display when transport is landing.
//;;   * ```order``` Order to give to newly landed droids
//;;   * ```data``` Order data.
//;; 	 __NOTE:__ the game engine doesn't seem to support two simultaneous
//;; 	incoming transporters for the same player. If a transporter is already
//;; 	on map, it will be correctly queued up and sent later.
//;;
function camSendReinforcement(player, position, list, kind, data)
{
	var pos = camMakePos(position);
	var order = CAM_ORDER_ATTACK;
	var order_data = { regroup: false, count: -1 };
	if (camDef(data) && camDef(data.order))
	{
		order = data.order;
		if (camDef(data.data))
		{
			order_data = data.data;
		}
	}
	switch(kind)
	{
		case CAM_REINFORCE_GROUND:
			var droids = [];
			for (var i = 0, l = list.length; i < l; ++i)
			{
				var prop = __camChangePropulsionOnDiff(list[i].prop);
				droids[droids.length] = addDroid(player, pos.x, pos.y,
				                        "Reinforcement", list[i].body,
				                        prop, "", "", list[i].weap);
			}
			camManageGroup(camMakeGroup(droids), order, order_data);
			break;
		case CAM_REINFORCE_TRANSPORT:
			__camTransporterQueue[__camTransporterQueue.length] = {
				player: player,
				position: position,
				list: list,
				data: data,
				order: order,
				order_data: order_data
			};
			__camDispatchTransporterSafe();
			break;
		default:
			camTrace("Unknown reinforcement type");
			break;
	}
}

//;; ## camSetBaseReinforcements(base label, interval, callback, kind, data)
//;;
//;; Periodically brings reinforcements to an enemy base, until the base is
//;; eliminated. Interval is the pause, in milliseconds, between reinforcements.
//;; Callback is name of a function that returns a list of droid templates to spawn,
//;; which may be different every time. Kind and data work similarly to
//;; ```camSendReinforcement()```.
//;; Use CAM_REINFORCE_NONE as kind to disable previously set reinforcements.
//;;
function camSetBaseReinforcements(blabel, interval, callback, kind, data)
{
	if (!camIsString(callback))
	{
		camDebug("Callback name must be a string (received", callback, ")");
	}
	var bi = __camEnemyBases[blabel];
	bi.reinforce_kind = kind;
	bi.reinforce_interval = interval;
	bi.reinforce_callback = callback;
	bi.reinforce_data = data;
}
