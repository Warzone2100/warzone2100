# libcampaign.js documentation

```libcampaign.js``` is a JavaScript library supplied with the game,
which contains reusable code for campaign scenarios. It is designed to
make scenario development as high-level and declarative as possible.
It also contains a few simple convenient wrappers.
Public API functions of ```libcampaign.js``` are prefixed with
```cam```. To use ```libcampaign.js```, add the following include
into your scenario code:

```javascript
include("script/campaign/libcampaign.js");
```

Also, most of the ```libcampaign.js``` features require some of the
game events handled by the library. Transparent JavaScript pre-hooks are
therefore injected into your global event handlers upon include. For
example, if ```camSetArtifacts()``` was called to let
```libcampaign.js``` manage scenario artifacts, then
```eventPickup()``` will be first handled by the library, and only then
your handler will be called, if any.
All of this happens automagically and does not normally require
your attention.

## camDef(something)

Returns false if something is JavaScript-undefined, true otherwise.

## camIsString(something)

Returns true if something is a string, false otherwise.

## camRand(max)

A non-synchronous random integer in range [0, max - 1].

## camCallOnce(function name)

Call a function by name, but only if it has not been called yet.

## camSafeRemoveObject(obj[, special effects?])

Remove a game object (by value or label) if it exists, do nothing otherwise.

## camMakePos(x, y | label | object)

Make a POSITION-like object, unless already done. Often useful
for making functions that would accept positions in both xx,yy and {x:xx,y:yy} forms.
Also accepts labels. If label of AREA is given, returns the center of the area.
If an existing object or label of such is given, returns a safe JavaScript
object containing its x, y and id.

## camDist(x1, y1, x2, y2 | pos1, x2, y2 | x1, y1, pos2 | pos1, pos2)

A wrapper for ```distBetweenTwoPoints()```.

## camPlayerMatchesFilter(player, filter)

A function to handle player filters in a way similar to
how JS API functions (eg. ```enumDroid(filter, ...)```) handle them.

## camRemoveDuplicates(array)

Remove duplicate items from an array.

## camCountStructuresInArea(label, [player])

Mimics wzscript's numStructsButNotWallsInArea().

## camChangeOnDiff(numeric value, [bool])

Change a numeric value based on campaign difficulty. If the second option is defined
then the opposite effect will occur on that value.

## camIsSystemDroid(game object)

Determine if the passed in object is a non-weapon based droid.

## camMakeGroup(what, filter)

Make a new group out of array of droids, single game object,
or label string, with fuzzy auto-detection of argument type.
Only droids would be added to the group. ```filter``` can be one of
a player index, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```;
defaults to ```ENEMIES```.

## camBreakAlliances()

Break alliances between all players.

## camEnableRes(list, player)

Grants research from the given list to player

## camCompleteRequiredResearch(items, player)

Grants research from the given list to player and also researches
the required research for that item.

## camMarkTiles(label | array of labels)

Mark area on the map by label(s), but only if debug mode is enabled.
Otherwise, remember what to mark in case it is going to be.

## camUnmarkTiles(label | array of labels)

No longer mark area(s) with given label(s) in debug mode.

## camDebug(string...)

Pretty debug prints - a wrapper around ```debug()```.
Prints a function call stack and the argument message,
prefixed with "DEBUG". Only use this function to indicate
actual bugs in the scenario script, because game shouldn't
print things when nothing is broken. If you want to keep
some prints around to make debugging easier without distracting
the user, use ```camTrace()```.

## camDebugOnce(string...)

Same as ```camDebug()```, but prints each message only once
during script lifetime.

## camTrace(string...)

Same as ```camDebug()```, but only warns in cheat mode.
Prefixed with "TRACE". It's safe and natural to keep ```camTrace()```
calls in your code for easier debugging.

## camTraceOnce(string...)

Same as ```camTrace()```, but prints each message only once
during script lifetime.

## isCheating()

Check if the player is in cheat mode.

## camSetArtifacts(artifacts)

Tell ```libcampaign.js``` to manage a certain set of artifacts.
The argument is a JavaScript map from object labels to artifact
description. If the label points to a game object, artifact will be
placed when this object is destroyed; if the label is a position, the
artifact will be placed instantly. Artifact description is a JavaScript
object with the following fields:

* ```tech``` The technology to grant when the artifact is recovered.
Note that this can be made into an array to make artifacts give out
more than one technology, if desired.
On __let me win__ cheat, all technologies stored in the artifacts
managed by this function are automatically granted.
Additionally, this function would call special event callbacks if they are
defined in your script, which should be named as follows,
where LABEL is the artifact label:
* ```camArtifactPickup_LABEL``` Called when the player picks up
	the artifact.

## camAllArtifactsPickedUp()

Returns true if all artifacts managed by ```libcampaign.js```
were picked up.

## camSetEnemyBases(bases)

Tell ```libcampaign.js``` to manage a certain set of enemy bases.
Management assumes auto-cleanup of leftovers on destruction, and also
counting how many bases have been successfully destroyed by the player.
The argument is a JavaScript map from group labels to base descriptions.
Each label points to a group of vital base structures. If no group label
with this name is defined, a group is created automatically
based on ```cleanup``` area and labeled. Base description
is a JavaScript object with the following optional fields:

* ```cleanup``` An area label to clean up features in once base is
	destroyed. If base id is not a group label, this field is required
	in order to auto-create the group of stuff in the area which doesn't
	qualify as a valid leftover.
* ```detectMsg``` A ```PROX_MSG``` message id to play when the base is detected.
* ```detectSnd``` A sound file to play when the base is detected.
* ```eliminateSnd``` A sound file to play when the base is eliminated.
	The sound is played in the center of the cleanup area,
	which needs to be defined.
* ```player``` If base is detected by cleanup area, only objects
	matching this player filter would be added to the base group or
	cleaned up. Note that this most likely disables feature cleanup.
Additionally, this function would call special event callbacks if they are
defined in your script, which should be named as follows,
where LABEL is the label of the base group:
* ```camEnemyBaseDetected_LABEL``` Called when the player sees an object from
	the enemy base group for the first time.
* ```camEnemyBaseEliminated_LABEL``` Called when the base is eliminated,
	right after leftovers were cleaned up.

## camDetectEnemyBase(base label)

Plays the "enemy base detected" message and places a beacon
for the enemy base defined by the label, as if the base
was actually found by the player.

## camAllEnemyBasesEliminated()

Returns true if all enemy bases managed by ```libcampaign.js```
are destroyed.

## camSendReinforcement(player, position, droids, kind, data)

Give a single bunch of droids (template list) for a player at
a position label. Kind can be one of:

* ```CAM_REINFORCE_GROUND``` Reinforcements magically appear
	on the ground.
* ```CAM_REINFORCE_TRANSPORT``` Reinforcements are unloaded from
	a transporter.
	__NOTE:__ the game engine doesn't seem to support two simultaneous
	incoming transporters for the same player. Avoid this at all costs!
	The following data fields are required:
  * ```entry``` Transporter entry position.
  * ```exit``` Transporter exit position.
  * ```message``` ```PROX_MSG``` to display when transport is landing.
  * ```order``` Order to give to newly landed droids
  * ```data``` Order data.
	 __NOTE:__ the game engine doesn't seem to support two simultaneous
	incoming transporters for the same player. If a transporter is already
	on map, it will be correctly queued up and sent later.

## camSetBaseReinforcements(base label, interval, callback, kind, data)

Periodically brings reinforcements to an enemy base, until the base is
eliminated. Interval is the pause, in milliseconds, between reinforcements.
Callback is name of a function that returns a list of droid templates to spawn,
which may be different every time. Kind and data work similarly to
```camSendReinforcement()```.
Use CAM_REINFORCE_NONE as kind to disable previously set reinforcements.

## camRemoveEnemyTransporterBlip()

Removes the last blip that an enemy transporter left behind, if any.

## camManageGroup(group, order, data)

Tell ```libcampaign.js``` to manage a certain group. The group
would be permanently managed depending on the high-level orders given.
For each order, data parameter is a JavaScript object that controls
different aspects of behavior. The order parameter is one of:

* ```CAM_ORDER_ATTACK``` Pursue human player, preferably around
	the given position. The following optional data object fields are
	available, none of which is required:
  * ```pos``` Position or list of positions to attack. If pos is a list,
		first positions in the list will be attacked first.
  * ```radius``` Circle radius around ```pos``` to scan for targets.
  * ```fallback``` Position to retreat.
  * ```morale``` An integer from 1 to 100. If that high percentage
		of the original group dies, fall back to the fallback position.
		If new droids are added to the group, it can recover and attack
		again.
  * ```count``` Override size of the original group. If unspecified,
		number of droids in the group at call time. Retreat on low morale
		and regroup is calculated against this value.
  * ```repair``` Health percentage to fall back to repair facility,
		if any.
  * ```regroup``` If set to true, the group will not move forward unless
		it has at least ```count``` droids in its biggest cluster.
		If ```count``` is set to -1, at least 2/3 of group's droids should be in
		the biggest cluster.
* ```CAM_ORDER_DEFEND``` Protect the given position. If too far, retreat
	back there ignoring fire. The following data object fields are
	available:
  * ```pos``` Position to defend.
  * ```radius``` Circle radius around ```pos``` to scan for targets.
  * ```count``` Override size of the original group. If unspecified,
		number of droids in the group at call time. Regroup is calculated
		against this value.
  * ```repair``` Health percentage to fall back to repair facility,
		if any.
  * ```regroup``` If set to true, the group will not move forward unless
		it has at least ```count``` droids in its biggest cluster.
		If ```count``` is set to -1, at least 2/3 of group's droids should be in
		the biggest cluster.
* ```CAM_ORDER_PATROL``` Move droids randomly between a given list of
	positions. The following data object fields are available:
  * ```pos``` An array of positions to patrol between.
  * ```interval``` Change positions every this many milliseconds.
  * ```count``` Override size of the original group. If unspecified,
		number of droids in the group at call time. Regroup is calculated
		against this value.
  * ```repair``` Health percentage to fall back to repair facility,
		if any.
  * ```regroup``` If set to true, the group will not move forward unless
		it has at least ```count``` droids in its biggest cluster.
		If ```count``` is set to -1, at least 2/3 of group's droids should be in
		the biggest cluster.
* ```CAM_ORDER_COMPROMISE``` Same as CAM_ORDER_ATTACK, just stay near the
	last (or only) attack position instead of looking for the player
	around the whole map. Useful for offworld missions,
	with player's LZ as the final position. The following data object fields
	are available:
  * ```pos``` Position or list of positions to compromise. If pos is a list,
		first positions in the list will be compromised first.
  * ```radius``` Circle radius around ```pos``` to scan for targets.
  * ```count``` Override size of the original group. If unspecified,
		number of droids in the group at call time. Regroup is calculated
		against this value.
  * ```repair``` Health percentage to fall back to repair facility,
		if any.
  * ```regroup``` If set to true, the group will not move forward unless
		it has at least ```count``` droids in its biggest cluster.
		If ```count``` is set to -1, at least 2/3 of group's droids should be in
		the biggest cluster.
* ```CAM_ORDER_FOLLOW``` Assign the group to commander. The sub-order
	is defined to be given to the commander. When commander dies,
	the group continues to execute the sub-order. The following data object
	fields are available:
  * ```droid``` Commander droid label.
  * ```order``` The order to give to the commander.
  * ```data``` Data of the commander's order.
  * ```repair``` Health percentage to fall back to repair facility, if any.

## camStopManagingGroup(group)

Tell ```libcampaign.js``` to stop managing a certain group.

## camManageTrucks(player)

Manage trucks for an AI player. This assumes recapturing oils and
rebuilding destroyed trucks in factories, the latter is implemented
via ```camQueueDroidProduction()``` mechanism.

## camQueueBuilding(player, stat[, pos])

Assuming truck management is enabled for the player, issue an order
to build a specific building near a certain position. The order
would be issued once as soon as a free truck becomes available. It will
not be re-issued in case the truck is destroyed before the building
is finished. If position is unspecified, the building would be built
near the first available truck. Otherwise, position may be a label
or a POSITION-like object.

## camOrderToString(order)

Print campaign order as string, useful for debugging.

## camSetFactories(factories)

Tell ```libcampaign.js``` to manage a certain set of enemy factories.
Management assumes producing droids, packing them into groups and
executing orders once the group becomes large-enough.
The argument is a JavaScript map from group labels to factory descriptions.
Each label points to a factory object. Factory description

is a JavaScript object with the following fields:
* ```assembly``` A rally point position label, where the group would
	gather.
* ```order``` An order to execute for every group produced in the
	factory. Same as the order parameter for ```camManageGroup()```.
* ```data``` Order data. Same as the data parameter for
	```camManageGroup()```.
* ```groupSize``` Number of droids to produce before executing the order.
	Also, if order is ```CAM_ORDER_ATTACK```, data.count defaults to this value.
* ```maxSize``` Halt production when reaching that many droids in the
	factory group. Resume when some droids die. Unlimited if unspecified.
* ```throttle``` If defined, produce droids only every that many
	milliseconds, and keep the factory idle between ticks.
* ```group``` If defined, make the factory manage this group,
	otherwise create a new empty group to manage.
	Droids produced in the factory would automatically be
	added to the group, and order and data parameters
	would be applied to this group.
* ```templates``` List of droid templates to produce in the factory.
	Each template is a JavaScript object with the following fields:
  * ```body``` Body stat name.
  * ```prop``` Propulsion stat name.
  * ```weap``` Weapon stat name. Only single-turret droids are
		currently supported.
	Note that all template components are automatically made available
	to the factory owner.
Factories won't start production immediately; call
```camEnableFactory()``` to turn them on when necessary.

## camSetFactoryData(factory label, factory description)

Similar to ```camSetFactories()```, but one factory at a time.
If the factory was already managing a group of droids, it keeps
managing it. If a new group is specified in the description,
the old group is merged into it. NOTE: This function disables the
factory. You would need to call ```camEnableFactory()``` again.

## camEnableFactory(factory label)

Enable a managed factory by the given label. Once the factory is enabled,
it starts producing units and executing orders as given.

## camQueueDroidProduction(player, template)

Queues up an extra droid template for production. It would be produced
in the first factory that is capable of producing it, at the end of
its production loop, first queued first served.

## camSetPropulsionTypeLimit(number)

On hard and insane the propulsion type can be limited with this. For type II
pass in 2, and for type III pass in 3. Hard defaults to type II and
insane defaults to type III. If nothing is passed in then the type
limit will match what is in templates.json.

## camNextLevel(next level)

A wrapper around ```loadLevel()```. Remembers to give bonus power
for completing the mission faster.

## camSetStandardWinLossConditions(kind, nextLevel, data)

Set victory and defeat conditions to one of the common
options. On victory, load nextLevel. The extra data parameter
contains extra data required to define some of the victory
conditions. The following options are available:

* ```CAM_VICTORY_STANDARD``` Defeat if all ground factories
	and construction droids are lost, or on mission timeout.
	Victory when all enemies are destroyed and all artifacts
	are recovered.
* ```CAM_VICTORY_PRE_OFFWORLD``` Defeat on timeout. Victory on
	transporter launch, then load the sub-level.
* ```CAM_VICTORY_OFFWORLD``` Defeat on timeout or all units lost.
	Victory when all artifacts are recovered and either all enemies
	are dead (not just bases) or all droids are at the LZ.
	Also automatically handles the "LZ compromised" message,
	which is why it needs to know reinforcement interval to restore.
	The following data parameter fields are available:
  * ```area``` The landing zone to return to.
  * ```message``` The "Return to LZ" message ID. Optional.
  * ```reinforcements``` Reinforcements interval, in seconds.
For standard and offworld victory, some extra data parameters can be defined:
* ```callback``` A function callback to check for extra win/loss
		conditions. Return values are interpreted as follows:
  * __false__ means instant defeat ("objective failed"),
  * __true__ means victory as long as other standard victory
		conditions are met,
  * __undefined__ means suppress
		other victory checks ("clearly not won yet").
* ```victoryVideo``` Pass in the name of a video string here
			and it will be played before attempting to load the next level.
For offworld victory, some more extra data parameters can be defined:
* ```retlz``` Force the player to return to the LZ area:
  * __false__ mission does not require a LZ return,
  * __true__ mission requires all units to be at LZ to win.
* ```annihilate``` Player must destroy every thing on map to win:
  * __false__ mission does not require everything destroyed,
  * __true__ mission requires total map annihilation.
* ```eliminateBases``` Instant win when all enemy units and bases are destroyed:
  * __false__ does not require all bases to be destroyed,
  * __true__ requires all bases destroyed.

## camIsTransporter(game object)

Determine if the object is a transporter.

## camSetupTransport(place x, place y, exit x, exit y)

A convenient function for placing the standard campaign transport
for loading in pre-away missions. The exit point for the transport
is set up as well.

## camNewGroup()

A saveload safe version of newGroup() so as not to create group ID clashes.

## camInNeverGroup(droid)

check if this droid is forced to never group.

## camNeverGroupDroid(what, [filter])

A means to not auto group some droids.

## camPlayVideos(videos)

If videos is an array, queue up all of them for immediate playing. This
function will play one video sequence should one be provided. Also,
should a sound file be in a string (pcvX.ogg)  __camEnqueueVideos() will recognize it
as a sound to play before a video. Of which is only supported when passed as
an array.

## camAreaEvent(label, function(droid))

Implement eventArea<label> in a debugging-friendly way. The function
marks the area until the event is triggered, and traces entering the area
in the TRACE log.

