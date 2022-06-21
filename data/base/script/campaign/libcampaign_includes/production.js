
////////////////////////////////////////////////////////////////////////////////
// Factory production management.
////////////////////////////////////////////////////////////////////////////////

// To use this feature, call camSetFactories() with a list of factories.
// This assumes tactical management of groups of units produced from them.
// Factories won't start production immediately; call camEnableFactory()
// to turn them on.

/**
 * ## camSetFactories(factories)
 *
 * Tell `libcampaign.js` to manage a certain set of enemy factories.
 * Management assumes producing droids, packing them into groups and
 * executing orders once the group becomes large-enough.
 * The argument is a JavaScript map from group labels to factory descriptions.
 * Each label points to a factory object.
 * Factory description is a JavaScript object with the following fields:
 * * `assembly` A rally point position label, where the group would gather.
 * * `order` An order to execute for every group produced in the factory. Same as the order parameter for `camManageGroup()`.
 * * `data` Order data. Same as the data parameter for `camManageGroup()`.
 * * `groupSize` Number of droids to produce before executing the order.
 *   Also, if order is `CAM_ORDER_ATTACK`, data.count defaults to this value.
 * * `maxSize` Halt production when reaching that many droids in the factory group.
 *   Resume when some droids die. Unlimited if unspecified.
 * * `throttle` If defined, produce droids only every that many milliseconds, and keep the factory idle between ticks.
 * * `group` If defined, make the factory manage this group, otherwise create a new empty group to manage.
 *   Droids produced in the factory would automatically be added to the group,
 *   and order and data parameters would be applied to this group.
 * * `templates` List of droid templates to produce in the factory.
 *   Each template is a JavaScript object with the following fields:
 *   * `body` Body stat name.
 *   * `prop` Propulsion stat name.
 *   * `weap` Weapon stat name. Only single-turret droids are currently supported.
 *   Note that all template components are automatically made available to the factory owner.
 * Factories won't start production immediately; call `camEnableFactory()` to turn them on when necessary.
 * @param {Object} factories
 * @returns {void}
 */
function camSetFactories(factories)
{
	for (const factoryLabel in factories)
	{
		camSetFactoryData(factoryLabel, factories[factoryLabel]);
	}
}

/**
 * ## camSetFactoryData(factoryLabel, factoryData)
 *
 * Similar to `camSetFactories()`, but one factory at a time.
 * If the factory was already managing a group of droids, it keeps managing it.
 * If a new group is specified in the description, the old group is merged into it.
 * NOTE: This function disables the factory. You would need to call `camEnableFactory()` again.
 * @param {string} factoryLabel
 * @param {Object} factoryData
 * @returns {void}
 */
function camSetFactoryData(factoryLabel, factoryData)
{
	var structure = getObject(factoryLabel);
	if (!camDef(structure) || !structure)
	{
		// Not an error! It's ok if the factory is already destroyed
		// when its data was updated.
		camTrace("Factory", factoryLabel, "not found");
		return;
	}
	// remember the old factory group, if any
	var droids = [];
	if (camDef(__camFactoryInfo[factoryLabel]))
	{
		droids = enumGroup(__camFactoryInfo[factoryLabel].group);
	}
	__camFactoryInfo[factoryLabel] = factoryData;
	var fi = __camFactoryInfo[factoryLabel];
	if (!camDef(fi.data))
	{
		fi.data = {};
	}
	fi.enabled = false;
	fi.state = 0;
	if (!camDef(fi.group))
	{
		fi.group = camNewGroup();
	}
	for (let i = 0, l = droids.length; i < l; ++i)
	{
		var droid = droids[i];
		groupAdd(fi.group, droid);
	}
	if (!camDef(fi.data.count))
	{
		fi.data.count = fi.groupSize;
	}
}

/**
 * ## camEnableFactory(factoryLabel)
 *
 * Enable a managed factory by the given label.
 * Once the factory is enabled, it starts producing units and executing orders as given.
 * @param {string} factoryLabel
 * @returns {void}
 */
function camEnableFactory(factoryLabel)
{
	var fi = __camFactoryInfo[factoryLabel];
	if (!camDef(fi) || !fi)
	{
		camDebug("Factory not managed", factoryLabel);
		return;
	}
	if (fi.enabled)
	{
		// safe, no error
		camTrace("Factory", factoryLabel, "enabled again");
		return;
	}
	camTrace("Enabling", factoryLabel);
	fi.enabled = true;
	var obj = getObject(factoryLabel);
	if (!camDef(obj) || !obj)
	{
		camTrace("Factory", factoryLabel, "not found, probably already dead");
		return;
	}
	__camContinueProduction(factoryLabel);
	__camFactoryUpdateTactics(factoryLabel);
}

/**
 * ## camQueueDroidProduction(playerId, template)
 *
 * Queues up an extra droid template for production.
 * It would be produced in the first factory that is capable of producing it,
 * at the end of its production loop, first queued first served.
 * @param {number} playerId
 * @param {Object} template
 * @returns {void}
 */
function camQueueDroidProduction(playerId, template)
{
	if (!camDef(__camFactoryQueue[playerId]))
	{
		__camFactoryQueue[playerId] = [];
	}
	__camFactoryQueue[playerId][__camFactoryQueue[playerId].length] = template;
}

/**
 * ## camSetPropulsionTypeLimit([limit])
 *
 * On hard and insane the propulsion type can be limited with this.
 * For type II pass in `2`, and for type III pass in `3`. Hard defaults to type II and insane defaults to type III.
 * If nothing is passed in then the type limit will match what is in templates.json.
 * @param {number} [limit]
 * @returns {void}
 */
function camSetPropulsionTypeLimit(limit)
{
	if (!camDef(limit))
	{
		__camPropulsionTypeLimit = "NO_USE";
	}
	else if (limit === 2)
	{
		__camPropulsionTypeLimit = "02";
	}
	else if (limit === 3)
	{
		__camPropulsionTypeLimit = "03";
	}
}

/**
 * ## camUpgradeOnMapTemplates(template1, template2, playerId[, excluded])
 *
 * Search for `template1`, save its coordinates, remove it, and then replace with it with `template2`.
 * Template objects are expected to follow the component properties as used in `templates.js`.
 * A fourth parameter can be specified to ignore specific object IDs.
 * Useful if a droid is assigned to an object label. It can be either an array or a single ID number.
 * @param {Object} template1
 * @param {Object} template2
 * @param {number} playerId
 * @param {number|number[]} [excluded]
 * @returns {void}
 */
function camUpgradeOnMapTemplates(template1, template2, playerId, excluded)
{
	if (!camDef(template1) || !camDef(template2) || !camDef(playerId))
	{
		camDebug("Not enough parameters specified for upgrading on map templates");
		return;
	}

	var droidsOnMap = enumDroid(playerId);

	for (let i = 0, l = droidsOnMap.length; i < l; ++i)
	{
		var dr = droidsOnMap[i];
		if (!camDef(dr.weapons[0]))
		{
			continue; //don't handle systems
		}
		var body = dr.body;
		var prop = dr.propulsion;
		var weap = dr.weapons[0].name;
		var skip = false;
		if (body === template1.body && prop === template1.prop && weap === template1.weap)
		{
			//Check if this object should be excluded from the upgrades
			if (camDef(excluded))
			{
				if (excluded instanceof Array)
				{
					for (let j = 0, c = excluded.length; j < c; ++j)
					{
						if (dr.id === excluded[j])
						{
							skip = true;
							break;
						}
					}
					if (skip === true)
					{
						continue;
					}
				}
				else if (dr.id === excluded)
				{
					continue;
				}
			}

			//Replace it
			let droidInfo = {x: dr.x, y: dr.y, name: dr.name};
			camSafeRemoveObject(dr, false);
			addDroid(playerId, droidInfo.x, droidInfo.y, droidInfo.name, template2.body,
				__camChangePropulsionOnDiff(template2.prop), "", "", template2.weap);
		}
	}
}

//////////// privates

function __camFactoryUpdateTactics(flabel)
{
	var fi = __camFactoryInfo[flabel];
	if (!fi.enabled)
	{
		camDebug("Factory", flabel, "was not enabled");
		return;
	}
	var droids = enumGroup(fi.group);
	if (droids.length >= fi.groupSize)
	{
		camManageGroup(fi.group, fi.order, fi.data);
		fi.group = camNewGroup();
	}
	else
	{
		var pos = camMakePos(fi.assembly);
		if (!camDef(pos))
		{
			pos = camMakePos(flabel);
		}
		camManageGroup(fi.group, CAM_ORDER_DEFEND, { pos: pos });
	}
}

function __camAddDroidToFactoryGroup(droid, structure)
{
	// don't manage trucks in this function
	if (droid.droidType === DROID_CONSTRUCT)
	{
		return;
	}
	// FIXME: O(n) lookup here
	var flabel = getLabel(structure);
	if (!camDef(flabel) || !flabel)
	{
		return;
	}
	var fi = __camFactoryInfo[flabel];
	groupAdd(fi.group, droid);
	if (camDef(fi.assembly))
	{
		// this is necessary in case droid is regrouped manually
		// in the scenario code, and thus DORDER_DEFEND for assembly
		// will not be applied in __camFactoryUpdateTactics()
		var pos = camMakePos(fi.assembly);
		orderDroidLoc(droid, DORDER_MOVE, pos.x, pos.y);
	}
	__camFactoryUpdateTactics(flabel);
}

function __camChangePropulsionOnDiff(propulsion)
{
	if (difficulty <= MEDIUM)
	{
		return propulsion;
	}
	if (camDef(__camPropulsionTypeLimit) && __camPropulsionTypeLimit === "NO_USE")
	{
		return propulsion; //this mission don't want this feature then
	}

	var name = propulsion;
	var typeModifier = difficulty === HARD ? "02" : "03";
	const VALID_PROPS = [
		"CyborgLegs", "HalfTrack", "V-Tol", "hover", "tracked", "wheeled",
	];

	var lastTwo = name.substring(name.length - 2);
	if (lastTwo === "01" || lastTwo === "02" || lastTwo === "03")
	{
		name = name.substring(0, name.length - 2);
	}

	for (let i = 0, l = VALID_PROPS.length; i < l; ++i)
	{
		var currentProp = VALID_PROPS[i];
		if (name === currentProp)
		{
			//if hard difficulty and a future template has a type III then this will
			//ensure it stays type III.
			if (difficulty === HARD && lastTwo === "02")
			{
				typeModifier = "03";
			}
			//maybe a mission wants to set a limit on the highest propulsion type
			if (camDef(__camPropulsionTypeLimit))
			{
				typeModifier = __camPropulsionTypeLimit;
			}
			//return a stronger propulsion based on difficulty
			return currentProp.concat(typeModifier);
		}
	}

	//If all else fails then return the propulsion that came with the template
	return propulsion;
}

function __camBuildDroid(template, structure)
{
	if (!camDef(structure))
	{
		return false;
	}
	//if not a normal factory and the template is a constructor then keep it in the
	//queue until a factory can deal with it.
	if (template.weap === "Spade1Mk1" && structure.stattype !== FACTORY)
	{
		return false;
	}
	var prop = __camChangePropulsionOnDiff(template.prop);
	makeComponentAvailable(template.body, structure.player);
	makeComponentAvailable(prop, structure.player);
	makeComponentAvailable(template.weap, structure.player);
	var n = [ structure.name, structure.id, template.body, prop, template.weap ].join(" ");
	// multi-turret templates are not supported yet
	return buildDroid(structure, n, template.body, prop, "", "", template.weap);
}

//Check if an enabled factory can begin manufacturing something. Doing this
//by timer has the perk of not breaking production if something went wrong in
//cam_eventDroidBuilt (or the mere act of reloading saves).
function __checkEnemyFactoryProductionTick()
{
	for (const flabel in __camFactoryInfo)
	{
		if (getObject(flabel) !== null && __camFactoryInfo[flabel].enabled === true)
		{
			__camContinueProduction(flabel);
		}
	}
}

function __camContinueProduction(structure)
{
	var flabel;
	var struct;
	if (camIsString(structure))
	{
		flabel = structure;
		struct = getObject(flabel);
		if (!camDef(struct) || !struct)
		{
			camTrace("Factory not found");
			return;
		}
	}
	else
	{
		// FIXME: O(n) lookup here
		flabel = getLabel(structure);
		struct = structure;
	}
	if (!camDef(flabel) || !flabel)
	{
		return;
	}
	if (!structureIdle(struct))
	{
		return;
	}
	var fi = __camFactoryInfo[flabel];
	if (camDef(fi.maxSize) && groupSize(fi.group) >= fi.maxSize)
	{
		// retry later
		return;
	}
	if (camDef(fi.throttle) && camDef(fi.lastprod))
	{
		var throttle = gameTime - fi.lastprod;
		if (throttle < fi.throttle)
		{
			// do throttle
			return;
		}
	}
	// check factory queue after every loop
	if (fi.state === -1)
	{
		fi.state = 0;
		var p = struct.player;
		if (camDef(__camFactoryQueue[p]) && __camFactoryQueue[p].length > 0)
		{
			if (__camBuildDroid(__camFactoryQueue[p][0], struct))
			{
				__camFactoryQueue[p].shift();
				return;
			}
		}
	}
	__camBuildDroid(fi.templates[fi.state], struct);
	// loop through templates
	++fi.state;
	if (fi.state >= fi.templates.length)
	{
		fi.state = -1;
	}
	fi.lastprod = gameTime;
}
