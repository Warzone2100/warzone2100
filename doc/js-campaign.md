# `libcampaign.js` documentation

`libcampaign.js` is a JavaScript library supplied with the game,
which contains reusable code for campaign scenarios. It is designed to
make scenario development as high-level and declarative as possible.
It also contains a few simple convenient wrappers.
Public API functions of `libcampaign.js` are prefixed with `cam`.
To use `libcampaign.js`, add the following include into your scenario code:

```js
include("script/campaign/libcampaign.js");
```

Also, most of the `libcampaign.js` features require some of the game
events handled by the library. Transparent JavaScript pre-hooks are
therefore injected into your global event handlers upon include.
For example, if `camSetArtifacts()` was called to let `libcampaign.js`
manage scenario artifacts, then `eventPickup()` will be first handled
by the library, and only then your handler will be called, if any.
All of this happens automagically and does not normally require
your attention.

## camSetArtifacts(artifacts)

Tell `libcampaign.js` to manage a certain set of artifacts.
The argument is a JavaScript map from object labels to artifact description.
If the label points to a game object, artifact will be placed when this object
is destroyed; if the label is a position, the artifact will be placed instantly.
The label can point to a pre-existing feature artifact on the map too.
Artifact description is a JavaScript object with the following fields:
* `tech` The technology to grant when the artifact is recovered.
  Note that this can be made into an array to make artifacts give out
  more than one technology, if desired.
  On `let me win` cheat, all technologies stored in the artifacts
  managed by this function are automatically granted.
  Additionally, this function would call special event callbacks if they are defined
  in your script, which should be named as follows, where LABEL is the artifact label:
* `camArtifactPickup_LABEL` Called when the player picks up the artifact.

@param {Object} artifacts
@returns {void}

## camAddArtifact(artiLabel, artiTech)

Adds another artifact to be managed. Will override existing ones if the names match.

@param {String} artiLabel
@param {String|Array} artiTech
@returns {void}

## camDeleteArtifact(artiLabel)

Deletes the artifact from the list of managed artifacts.

@param {String} artiLabel
@returns {void}

## camAllArtifactsPickedUp()

Returns `true` if all artifacts managed by `libcampaign.js` were picked up.

@returns {boolean}

## camGetArtifacts()

Returns the labels of all existing artifacts.

@returns {Object[]}

## camSetEnemyBases([bases])

Tell `libcampaign.js` to manage a certain set of enemy bases.
Management assumes auto-cleanup of leftovers on destruction, and also
counting how many bases have been successfully destroyed by the player.
The argument is a JavaScript map from group labels to base descriptions.
Each label points to a group of vital base structures. If no group label
with this name is defined, a group is created automatically
based on `cleanup` area and labeled. Base description
is a JavaScript object with the following optional fields:
* `cleanup` An area label to clean up features in once base is destroyed.
  If base id is not a group label, this field is required in order to auto-create
  the group of stuff in the area which doesn't qualify as a valid leftover.
* `detectMsg` A `PROX_MSG` message id to play when the base is detected.
* `detectSnd` A sound file to play when the base is detected.
* `eliminateSnd` A sound file to play when the base is eliminated.
  The sound is played in the center of the cleanup area, which needs to be defined.
* `player` If base is detected by cleanup area, only objects matching
  this player filter would be added to the base group or cleaned up.
  Note that this most likely disables feature cleanup.
  Additionally, this function would call special event callbacks if they are defined in your script,
  which should be named as follows, where LABEL is the label of the base group:
* `camEnemyBaseDetected_LABEL` Called when the player sees an object from the enemy base group for the first time.
* `camEnemyBaseEliminated_LABEL` Called when the base is eliminated, right after leftovers were cleaned up.

@param {Object} [bases]
@returns {void}

## camDetectEnemyBase(baseLabel)

Plays the "enemy base detected" message and places a beacon for the enemy base
defined by the label, as if the base was actually found by the player.

@param {string} baseLabel
@returns {void}

## camAllEnemyBasesEliminated()

Returns `true` if all enemy bases managed by `libcampaign.js` are destroyed.

@returns {boolean}

## camMarkTiles(label|labels)

Mark area on the map by label(s), but only if debug mode is enabled.
Otherwise, remember what to mark in case it is going to be.

@param {string|string[]} label
@returns {void}

## camUnmarkTiles(label|labels)

No longer mark area(s) with given label(s) in debug mode.

@param {string|string[]} label
@returns {void}

## camDebug(...args)

Pretty debug prints - a wrapper around `debug()`.
Prints a function call stack and the argument message, prefixed with `DEBUG`.
Only use this function to indicate actual bugs in the scenario script,
because game shouldn't print things when nothing is broken.
If you want to keep some prints around to make debugging easier
without distracting the user, use `camTrace()`.

@param {...string} args
@returns {void}

## camDebugOnce(...args)

Same as `camDebug()`, but prints each message only once during script lifetime.

@param {...string} args
@returns {void}

## camTrace(...args)

Same as `camDebug()`, but only warns in cheat mode.
Prefixed with `TRACE`. It's safe and natural to keep `camTrace()` calls in your code for easier debugging.

@param {...string} args
@returns {void}

## camTraceOnce(...args)

Same as `camTrace()`, but prints each message only once during script lifetime.

@param {...string} args
@returns {void}

## camIsCheating()

Check if the player is in cheat mode.

@returns {boolean}

## camNewGroup()

A saveload safe version of `newGroup()` so as not to create group ID clashes.

@returns {number}

## camInNeverGroup(droid)

Check if this droid is forced to never group.

@param {Object} droid
@returns {boolean}

## camNeverGroupDroid(what[, playerFilter])

A means to not auto group some droids.

@param {string|Object|Object[]} what
@param {number} [playerFilter]
@returns {void}

## camAreaEvent(label, callback(droid))

Implement `eventArea${label}()` in a debugging-friendly way.
The function marks the area until the event is triggered,
and traces entering the area in the TRACE log.

@param {string} label
@param {Function} callback
@returns {void}

## camClassicMode()

Returns `true` if classic balance mod is enabled.

@returns {boolean}

## camDef(something)

Returns `false` if something is JavaScript-undefined, `true` otherwise.

@param {*} something
@returns {boolean}

## camIsString(something)

Returns `true` if something is a string, `false` otherwise.

@param {*} something
@returns {boolean}

## camRand(max)

A non-synchronous random integer in range [0, max - 1].

@param {number} max
@returns {number}

## camCallOnce(functionName)

Call a function by name, but only if it has not been called yet.

@param {string} functionName
@returns {void}

## camSafeRemoveObject(obj[, specialEffects])

Remove a game object (by value or label) if it exists, do nothing otherwise.

@param {string|Object} obj
@param {boolean} [specialEffects]
@returns {void}

## camMakePos(label|object|x[, y])

Make a `POSITION`-like object, unless already done.
Often useful for making functions that would accept positions in both `x,y` and `{x: x, y: y}` forms.
Also accepts labels. If label of `AREA` is given, returns the center of the area.
If an existing object or label of such is given, returns a safe JavaScript object containing its `x`, `y` and `id`.

@param {number|string|Object|undefined} x
@param {number} [y]
@returns {Object|undefined}

## camDist(x1, y1, x2, y2 | pos1, x2, y2 | x1, y1, pos2 | pos1, pos2)

A wrapper for `distBetweenTwoPoints()`.

@param {number|Object} x1
@param {number|Object} y1
@param {number} [x2]
@param {number} [y2]
@returns {number}

## camPlayerMatchesFilter(playerId, playerFilter)

A function to handle player filters in a way similar to how JS API functions (eg. `enumDroid(filter, ...)`) handle them.

@param {number} playerId
@param {number} playerFilter
@returns {boolean}

## camRemoveDuplicates(items)

Remove duplicate items from an array.

@param {*[]} items
@returns {*[]}

## camCountStructuresInArea(label[, playerFilter])

Mimics wzscript's `numStructsButNotWallsInArea()`.

@param {string} label
@param {number} [playerFilter]
@returns {number}

## camCleanTileOfObstructions(x, y | pos)

Obliterates player structures and features near the tile around certain coordinates.
Can be used for spawn locations or transport reinforcement spots. May not
delete very large objects like factories or skyscrapers.

@param {number|Object} x
@param {number} [y]
@returns {void}

## camChangeOnDiff(numericValue)

Change a numeric value based on campaign difficulty.

@param {number} numericValue
@returns {number}

## camAllowInsaneSpawns()

Allow additional Insane difficulty (or higher) spawns and behavior.

@returns {boolean}

## camIsSystemDroid(gameObject)

Determine if the passed in object is a non-weapon based droid.

@param {Object} gameObject
@returns {boolean}

## camMakeGroup(what[, playerFilter])

Make a new group out of array of droids, single game object, or label string,
with fuzzy auto-detection of argument type.
Only droids would be added to the group. `playerFilter` can be one of a
player index, `ALL_PLAYERS`, `ALLIES` or `ENEMIES`; defaults to `ENEMIES`.

@param {string|Object|Object[]} what
@param {number} [playerFilter]
@returns {number|void}

## camBreakAlliances()

Break alliances between all players.

@returns {void}

## camIsWaterPropulsion(propulsion)

Check if a propulsion can traverse water tiles. Until the Stats object can tell us this
information, it simply uses a very basic name check against the default hover propulsions.

@param {String} propulsion
@returns {Boolean}

## camNearInaccessibleAreas(start, destination [, propulsion [, distance]])

Determine if a start position can reach a destination position within the limits of
the chosen propulsion, and if there are nearby cliffs/water tiles nearby within a certain distance.
if `destination` is undefined it will cause this function to just scan around the start position.

@param {Object} start
@param {Object} destination
@param {String} propulsion
@param {Number} distance
@returns {Boolean}

## camGenerateRandomMapEdgeCoordinate(reachPosition [, propulsion [, distFromReach [, scanObjectRadius]]])

Returns a random coordinate anywhere on the edge of the map that reaches a position.
`reachPosition` may be undefined if you just want a random edge coordinate, without object scans.
Which can be useful for spawning transporter entry/exit points or VTOL spawn positions.
`scanObjectRadius` may be defined to scan possible spawn points for nearby objects,
and should be above one tile if there are large skyscrapers at the edges of some maps.

@param {Object} reachPosition
@param {String} propulsion
@param {Number} distFromReach
@param {Number} scanObjectRadius
@returns {Object}

## camGenerateRandomMapCoordinate(reachPosition [, propulsion [, distFromReach [, scanObjectRadius, [, avoidNearbyCliffs]]]])

Returns a random coordinate anywhere on the map.

@param {Object} reachPosition
@param {String} propulsion
@param {Number} distFromReach
@param {Number} scanObjectRadius
@param {Boolean} avoidNearbyCliffs
@returns {Object}

## camDiscoverCampaign()

Figures out what campaign we are in without reliance on the source at all.

@returns {number}

## camGetRankThreshold(rank [, command [, player]])

Returns the rank threshold for a given rank.

@param {String|Number} rank
@param {Boolean} command
@param {Number} player
@returns {number}

## camSetExpLevel(rank)

Sets what rank will be used for the AI when it creates units. Can be a rank threshold
index or the name of the rank.

@param {Number|String} rank
@returns {void}

## camSetOnMapEnemyUnitExp()

Sets all non-player units to the chosen rank set through camSetExpLevel().

@returns {void}

## camNexusLaugh()

Play a random NEXUS laugh.

@returns {void}

## camAbsorbPlayer([who[, to]])

Completely give all of player `who` droids and structures to player `to`.
Will default to `CAM_HUMAN_PLAYER` and `CAM_NEXUS` respectively.

@param {number} [who]
@param {number} [to]
@returns {void}

## camHackIntoPlayer([player[, to]])

Steal a droid or structure from a player if the NEXUS hack state is active.
Will default to `CAM_HUMAN_PLAYER` and `CAM_NEXUS` respectively.

@param {number} [player]
@param {number} [to]
@returns {void}

## camSetNexusState(flag)

Turn on/off the NEXUS hacking state feature.

@param {boolean} flag
@returns {void}

## camGetNexusState()

Returns the activation state of the NEXUS hacking feature.

@returns {boolean}

## camSetFactories(factories)

Tell `libcampaign.js` to manage a certain set of enemy factories.
Management assumes producing droids, packing them into groups and
executing orders once the group becomes large-enough.
The argument is a JavaScript map from group labels to factory descriptions.
Each label points to a factory object.
Factory description is a JavaScript object with the following fields:
* `assembly` A rally point position label, where the group would gather.
* `order` An order to execute for every group produced in the factory. Same as the order parameter for `camManageGroup()`.
* `data` Order data. Same as the data parameter for `camManageGroup()`.
* `groupSize` Number of droids to produce before executing the order.
  Also, if order is `CAM_ORDER_ATTACK`, data.count defaults to this value.
* `maxSize` Halt production when reaching that many droids in the factory group.
  Resume when some droids die. Unlimited if unspecified.
* `throttle` If defined, produce droids only every that many milliseconds, and keep the factory idle between ticks.
* `group` If defined, make the factory manage this group, otherwise create a new empty group to manage.
  Droids produced in the factory would automatically be added to the group,
  and order and data parameters would be applied to this group.
* `templates` List of droid templates to produce in the factory.
  Each template is a JavaScript object with the following fields:
  * `body` Body stat name.
  * `prop` Propulsion stat name.
  * `weap` Weapon stat name. Only single-turret droids are currently supported.
  Note that all template components are automatically made available to the factory owner.
Factories won't start production immediately; call `camEnableFactory()` to turn them on when necessary.

@param {Object} factories
@returns {void}

## camSetFactoryData(factoryLabel, factoryData)

Similar to `camSetFactories()`, but one factory at a time.
If the factory was already managing a group of droids, it keeps managing it.
If a new group is specified in the description, the old group is merged into it.
NOTE: This function disables the factory. You would need to call `camEnableFactory()` again.

@param {string} factoryLabel
@param {Object} factoryData
@returns {void}

## camEnableFactory(factoryLabel)

Enable a managed factory by the given label.
Once the factory is enabled, it starts producing units and executing orders as given.

@param {string} factoryLabel
@returns {void}

## camQueueDroidProduction(playerId, template)

Queues up an extra droid template for production.
It would be produced in the first factory that is capable of producing it,
at the end of its production loop, first queued first served.

@param {number} playerId
@param {Object} template
@returns {void}

## camSetPropulsionTypeLimit([limit])

This function can automatically augment units to use Type I/II/III propulsions.
If nothing or zero is passed in then the type limit will match what is in templates.json.

@param {number} [limit]
@returns {void}

## camUpgradeOnMapTemplates(template1, template2, playerId[, excluded])

Search for `template1`, save its coordinates, remove it, and then replace with it with `template2`.
Template objects are expected to follow the component properties as used in `templates.js`.
A fourth parameter can be specified to ignore specific object IDs.
Useful if a droid is assigned to an object label. It can be either an array or a single ID number.

@param {Object} template1
@param {Object} template2
@param {number} playerId
@param {number|number[]} [excluded]
@returns {void}

## camSendReinforcement(playerId, position, templates, kind[, data])

Give a single bunch of droids (template list) for a player at a position label. Kind can be one of:
* `CAM_REINFORCE_NONE` Reinforcements are skipped this call.
* `CAM_REINFORCE_GROUND` Reinforcements magically appear on the ground.
* `CAM_REINFORCE_TRANSPORT` Reinforcements are unloaded from a transporter.
  **NOTE:** the game engine doesn't seem to support two simultaneous incoming transporters for the same player.
  Avoid this at all costs!
  The following data fields are required:
  * `entry` Transporter entry position.
  * `exit` Transporter exit position.
  * `message` `PROX_MSG` to display when transport is landing.
  * `order` Order to give to newly landed droids
  * `data` Order data.
  **NOTE:** the game engine doesn't seem to support two simultaneous incoming transporters for the same player.
  If a transporter is already on map, it will be correctly queued up and sent later.

@param {number} playerId
@param {string|Object|undefined} position
@param {Object[]} templates
@param {number} kind
@param {Object} [data]
@returns {void}

## camSendGenericSpawn(kind, player, condition, position, unitList, minimumUnits, maxRandoms)

A simple means to send a group of units, either by spawn or transporter, on a simple attack order.
Creating VTOLs this way isn't advised as they won't automatically retreat to a removal zone. However,
for maps with VTOL Rearming Pads present, it could be ok as long as the mission script is careful.
`kind`: Works similar to `camSendReinforcement()`.
`player`: The player that will own these units.
`condition`: Works similar to `camReinforcementConditionPasses()`.
`position`: An object that contains coordinate data, such as {x: x, y: y}, to tell where units will appear.
  May be a label or even an array of labels or objects containing coordinates, and if so, will be randomly selected.
`unitList`: An array of units that will randomly be chosen from, or, a single template.
May also be an object like so: {units: array_of_units|single_template, appended: array_of_units|single_template}.
`appended` will always additionally be added to the randomly selected units.
`minimumUnits`: The absolute minimum units this spawn will create.
`maxRandoms`: How many units will randomly be added to the minimumUnits.

@param {number} type
@param {number} player
@param {number} condition
@param {object} position
@param {array} unitList
@param {number} minimumUnits
@param {number} maxRandoms
@returns {boolean}

## camSetBaseReinforcements(baseLabel, interval, callbackName, kind, data)

Periodically brings reinforcements to an enemy base, until the base is eliminated.
Interval is the pause, in milliseconds, between reinforcements.
Callback is name of a function that returns a list of droid templates to spawn,
which may be different every time. Kind and data work similarly to `camSendReinforcement()`.
Use `CAM_REINFORCE_NONE` as kind to disable previously set reinforcements.

@param {string} baseLabel
@param {number} interval
@param {string} callbackName
@param {number} kind
@param {Object} data
@returns {void}

## camReinforcementConditionPasses(condition, player)

Determines if a generic reinforcement condition will allow a spawn to proceed.
`condition`: Basic conditions to not spawn anything, can be one of:
  * `CAM_REINFORCE_CONDITION_NONE` or `undefined`: Do not use a condition, will always attempt a spawn.
  * `CAM_REINFORCE_CONDITION_BASES`: Do not spawn if all bases are eliminated.
  * `CAM_REINFORCE_CONDITION_UNITS`: Do not spawn if `player` has zero units.
  * An `Object` {condition: `CAM_REINFORCE_CONDITION_OBJECT`, object: `label_of_object`} or `label`: spawns only if object is alive.
  * `CAM_REINFORCE_CONDITION_ARTIFACTS`: Do not spawn if the player has picked up all artifacts.
`player`: The player to check conditions for. Doesn't need to be specified except for the units condition.

@param {Number|Object} condition
@param {Number} player
@returns {Boolean}

## camEnableRes(researchIds, playerId)

Grants research from the given list to player

@param {string[]} researchIds
@param {number} playerId
@returns {void}

## camCompleteRequiredResearch(researchIds, playerId)

Grants research from the given list to player and also researches the required research for that item.

@param {string[]} researchIds
@param {number} playerId
@returns {void}

## camClassicResearch(researchIds, playerId)

Grants research from the given list to player based on the "classic balance" variant.

@param {string[]} researchIds
@param {number} playerId
@returns {void}

## camManageGroup(group, order, data)

Tell `libcampaign.js` to manage a certain group. The group would
be permanently managed depending on the high-level orders given.
For each order, data parameter is a JavaScript object that controls
different aspects of behavior. The order parameter is one of:
* `CAM_ORDER_ATTACK` Pursue human player, preferably around the given position.
  The following optional data object fields are available, none of which is required:
  * `pos` Position or list of positions to attack. If pos is a list, first positions in the list will be attacked first.
  * `radius` Circle radius around `pos` to scan for targets.
  * `fallback` Position to retreat.
  * `morale` An integer from `1` to `100`. If that high percentage of the original group dies,
    fall back to the fallback position. If new droids are added to the group, it can recover and attack again.
  * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
    Retreat on low morale and regroup is calculated against this value.
  * `repair` Health percentage to fall back to repair facility, if any.
  * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
    If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
* `CAM_ORDER_DEFEND` Protect the given position. If too far, retreat back there ignoring fire.
  The following data object fields are available:
  * `pos` Position to defend.
  * `radius` Circle radius around `pos` to scan for targets.
  * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
    Regroup is calculated against this value.
  * `repair` Health percentage to fall back to repair facility, if any.
  * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
    If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
* `CAM_ORDER_PATROL` Move droids randomly between a given list of positions. The following data object fields are available:
  * `pos` An array of positions to patrol between.
  * `interval` Change positions every this many milliseconds.
  * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
    Regroup is calculated against this value.
  * `repair` Health percentage to fall back to repair facility, if any.
  * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
    If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
  * `patrolType` Type of patrol behavior. Defaults to `CAM_PATROL_RANDOM` where the group randomly chooses a patrol position.
    `CAM_PATROL_CYCLE` forces the group to loop through the entire list of patrol positions one after the other.
  * `reactToAttack` Defaults to false and can be used to break the group out of patrol and
     into a `CAM_ORDER_ATTACK` state, if the group is attacked.
* `CAM_ORDER_COMPROMISE` Same as `CAM_ORDER_ATTACK`, just stay near the last (or only)
  attack position instead of looking for the player around the whole map. Useful for offworld missions,
  with player's LZ as the final position. The following data object fields are available:
  * `pos` Position or list of positions to compromise.
    If pos is a list, first positions in the list will be compromised first.
  * `radius` Circle radius around `pos` to scan for targets.
  * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
    Regroup is calculated against this value.
  * `repair` Health percentage to fall back to repair facility, if any.
  * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
    If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
* `CAM_ORDER_FOLLOW` Assign the group to commander. The sub-order is defined to be given to the commander.
  When commander dies, the group continues to execute the sub-order. The following data object fields are available:
  * `droid` Commander droid label.
  * `order` The order to give to the commander.
  * `data` Data of the commander's order.
  * `repair` Health percentage to fall back to repair facility, if any.

@param {string} group
@param {number} order
@param {Object} data
@returns {void}

## camStopManagingGroup(group)

Tell `libcampaign.js` to stop managing a certain group.

@param {string} group
@returns {void}

## camOrderToString(order)

Print campaign order as string, useful for debugging.

@param {number} order
@returns {string}

## camSecondsToMilliseconds(seconds)

Returns the amount of milliseconds in `seconds`.

@param {number} seconds
@returns {number}

## camMinutesToMilliseconds(minutes)

Returns the amount of milliseconds in `minutes`.

@param {number} minutes
@returns {number}

## camMinutesToSeconds(minutes)

Returns the amount of seconds in `minutes`.

@param {number} minutes
@returns {number}

## camHoursToSeconds(hours)

Returns the amount of seconds in `hours`.

@param {number} hours
@returns {number}

## camSetMissionTimer(seconds)

Sets the mission time for a level. This acts as a wrapper for `setMissionTime()`
and is mostly used to either set the time value passed (as seconds) or infinite
time if the `infiniteTime` tweak option is active.

@param {number} time value in seconds
@returns {void}

## camIsTransporter(gameObject)

Determine if the object is a transporter.

@param {Object} gameObject
@returns {boolean}

## camSetupTransporter(placeX, placeY, exitX, exitY)

A convenient function for placing the standard campaign transport for loading in pre-away missions.
The exit point for the transport is set up as well.

@param {number} placeX
@param {number} placeY
@param {number} exitX
@param {number} exitY
@returns {void}

## camRemoveEnemyTransporterBlip()

Removes the last blip that an enemy transporter left behind, if any.

@returns {void}

## camManageTrucks(playerId [, allowOilCapture])

Manage trucks for an AI player. This assumes rebuilding destroyed trucks
in factories, which is implemented via the `camQueueDroidProduction()` mechanism.

@param {number} playerId
@param {boolean} allowOilCapture
@returns {void}

## camQueueBuilding(playerId, stat[, position])

Assuming truck management is enabled for the player, issue an order to build a specific building
near a certain position. The order would be issued once as soon as a free truck becomes available.
It will not be re-issued in case the truck is destroyed before the building is finished.
If position is unspecified, the building would be built near the first available truck.
Otherwise, position may be a label or a `POSITION`-like object.

@param {number} playerId
@param {string} stat
@param {string|Object} [position]
@returns {void}

## camNextLevel(nextLevel)

A wrapper around `loadLevel()`. Remembers to give bonus power for completing the mission faster.

@param {string} nextLevel
@returns {void}

## camSetStandardWinLossConditions(kind, nextLevel, data)

Set victory and defeat conditions to one of the common options. On victory, load nextLevel.
The extra data parameter contains extra data required to define some of the victory conditions.
The following options are available:
* `CAM_VICTORY_STANDARD` Defeat if all ground factories and construction droids are lost, or on mission timeout.
  Victory when all enemies are destroyed and all artifacts are recovered.
* `CAM_VICTORY_PRE_OFFWORLD` Defeat on timeout. Victory on transporter launch, then load the sub-level.
* `CAM_VICTORY_OFFWORLD` Defeat on timeout or all units lost.
  Victory when all artifacts are recovered and either all enemies are dead (not just bases) or all droids are at the LZ.
  Also automatically handles the "LZ compromised" message, which is why it needs to know reinforcement interval to restore.
  The following data parameter fields are available:
  * `area` The landing zone to return to.
  * `message` The "Return to LZ" message ID. Optional.
  * `playLzReminder` Play and show the "Return to LZ" message. Optional, enabled by default.
  * `reinforcements` Reinforcements interval, in seconds.
For standard and offworld victory, some extra data parameters can be defined:
* `callback` A function callback to check for extra win/loss conditions. Return values are interpreted as follows:
  * `false` means instant defeat ("objective failed"),
  * `true` means victory as long as other standard victory conditions are met,
  * `undefined` means suppress other victory checks ("clearly not won yet").
For offworld victory, some more extra data parameters can be defined:
* `retlz` Force the player to return to the LZ area:
  * `false` mission does not require a LZ return,
  * `true` mission requires all units to be at LZ to win.
* `annihilate` Player must destroy every thing on map to win:
  * `false` mission does not require everything destroyed,
  * `true` mission requires total map annihilation.
* `eliminateBases` Instant win when all enemy units and bases are destroyed:
  * `false` does not require all bases to be destroyed,
  * `true` requires all bases destroyed.

@param {string} kind
@param {string} nextLevel
@param {Object} data
@returns {void}

## camCheckExtraObjective()

Checks for extra win conditions defined in level scripts being met, if any.

@returns {boolean|undefined}

## camSetExtraObjectiveMessage(message)

Message(s) the mission script can set to further explain specific victory conditions.
Allows a single string or an array of strings.

@param {string|Object[]} message
@returns {void}

## camClearConsoleOnVictoryMessage(clear)

If the script wants to allow `__camSetupConsoleForVictoryConditions()` to clear the console.

@param {boolean} clear
@returns {void}

## camPlayVideos(data)

Formats for parameter `data`: `{video: "video string", type: MISS_MSG/CAMP_MSG, immediate: true/false}` OR
`["sound file", {video: "video string", type: MISS_MSG/CAMP_MSG, immediate: true/false}, ...]`
object property "immediate" is optional since most videos are immediate.
If videos is an array, queue up all of them for immediate playing.
This function will play one video sequence should one be provided.
Also, should a sound file be in a string (`pcvX.ogg`) `__camEnqueueVideos()` will recognize it
as a sound to play before a video. Of which is only supported when parameter `data` is an array.

@param {Object|Object[]} data
@returns {void}

## camSetVtolData(player, startPos, exitPos, templates, timer, [condition[, extras]])

Setup hit and runner VTOLs. NOTE: Will almost immediately spawn VTOLs upon calling this function.
`Player`: What player number the VTOLs will belong to.
`StartPos`: Starting position object where VTOLs will spawn. Can be an array. Use undefined for random map edge location.
`ExitPos`: Exit position object where VTOLs will despawn at.
`Templates`: An array of templates that the spawn uses.
`Timer`: How much time in milliseconds the VTOLs will wait to spawn again.
`Condition`: A game object label that will stop the spawn when it no longer exists. Can use `camReinforcementConditionPasses()` conditions too.
`Extras`: An object with possible members:
		`limit`: Numeric limit of a VTOL design in regards to the parameter Templates. May be an array paired to Templates.
		`alternate`: A boolean to force the spawn to use one of the designs at a time in parameter Templates.
		`altIdx`: Which design index the spawn will first cycle through the list of templates from.
		`minVTOLs`: Minimum amount of VTOLs that will spawn.
		`maxRandomVTOLs`: Random amount of VTOLs that will spawn in addition to minVTOLs.

@param {number} player
@param {Object|Object[]|undefined} startPos
@param {Object} exitPos
@param {Object[]} templates
@param {number} timer
@param {Object|Number} Condition
@param {Object} extras
@returns {void}

## camSetVtolSpawnState(state, identifier)

Sets the active status of a VTOL spawn point. The identifier can either be the
the index number or the label of the object that stops the spawn naturally.

@param {boolean} state
@param {number|string} identifier
@returns {void}

## camSetVtolSpawnStateAll(state)

Sets the active status of all VTOL spawns to `state`.

@param {boolean} state
@returns {void}

## camVtolCanDisappear(droid)

Checks if the given VTOL can fly off map if damage or ammo amount allows.

@param {Object} droid
@returns {Boolean}

