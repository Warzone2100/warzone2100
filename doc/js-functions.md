# Functions

This section describes functions that can be called from scripts to make
things happen in the game (usually called our script 'API').

## setTimer(function, milliseconds[, object])

Set a function to run repeated at some given time interval. The function to run
is the first parameter, and it _must be quoted_, otherwise the function will
be inlined. The second parameter is the interval, in milliseconds. A third, optional
parameter can be a **game object** to pass to the timer function. If the **game object**
dies, the timer stops running. The minimum number of milliseconds is 100, but such
fast timers are strongly discouraged as they may deteriorate the game performance.

```javascript
  function conDroids()
  {
     ... do stuff ...
  }
  // call conDroids every 4 seconds
  setTimer("conDroids", 4000);
```

## removeTimer(function)

Removes an existing timer. The first parameter is the function timer to remove,
and its name _must be quoted_.

## queue(function[, milliseconds[, object]])

Queues up a function to run at a later game frame. This is useful to prevent
stuttering during the game, which can happen if too much script processing is
done at once.  The function to run is the first parameter, and it
_must be quoted_, otherwise the function will be inlined.
The second parameter is the delay in milliseconds, if it is omitted or 0,
the function will be run at a later frame.  A third optional
parameter can be a **game object** to pass to the queued function. If the **game object**
dies before the queued call runs, nothing happens.

## profile(function[, arguments])
Calls a function with given arguments, measures time it took to evaluate the function,
and adds this time to performance monitor statistics. Transparently returns the
function's return value. The function to run is the first parameter, and it
_must be quoted_. (3.2+ only)

## namespace(prefix)
Registers a new event namespace. All events can now have this prefix. This is useful for
code libraries, to implement event that do not conflict with events in main code. This
function should be called from global; do not (for hopefully obvious reasons) put it
inside an event.

## include(file)
Includes another source code file at this point. You should generally only specify the filename,
not try to specify its path, here.

## getWeaponInfo(weapon id)

Return information about a particular weapon type. DEPRECATED - query the Stats object instead. (3.2+ only)

## resetLabel(label[, filter])

Reset the trigger on an label. Next time a unit enters the area, it will trigger
an area event. Next time an object or a group is seen, it will trigger a seen event.
Optionally add a filter on it in the second parameter, which can
be a specific player to watch for, or ALL_PLAYERS by default.
This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
## resetArea(label[, filter])
Reset the trigger on an area. Next time a unit enters the area, it will trigger
an area event. Optionally add a filter on it in the second parameter, which can
be a specific player to watch for, or ALL_PLAYERS by default.
This is a fast operation of O(log n) algorithmic complexity. DEPRECATED - use resetLabel instead. (3.2+ only)

## enumLabels([filter])

Returns a string list of labels that exist for this map. The optional filter
parameter can be used to only return labels of one specific type. (3.2+ only)

## addLabel(object, label)

Add a label to a game object. If there already is a label by that name, it is overwritten.
This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)

## removeLabel(label)

Remove a label from the game. Returns the number of labels removed, which should normally be
either 1 (label found) or 0 (label not found). (3.2+ only)

## getLabel(object)

Get a label string belonging to a game object. If the object has multiple labels, only the first
label found will be returned. If the object has no labels, null is returned.
This is a relatively slow operation of O(n) algorithmic complexity. (3.2+ only)

## getObject(label | x, y | type, player, id)

Fetch something denoted by a label, a map position or its object ID. A label refers to an area,
a position or a **game object** on the map defined using the map editor and stored
together with the map. In this case, the only argument is a text label. The function
returns an object that has a type variable defining what it is (in case this is
unclear). This type will be one of DROID, STRUCTURE, FEATURE, AREA, GROUP or POSITION.
The AREA has defined 'x', 'y', 'x2', and 'y2', while POSITION has only defined 'x' and 'y'.
The GROUP type has defined 'type' and 'id' of the group, which can be passed to enumGroup().
This is a fast operation of O(log n) algorithmic complexity. If the label is not found, an
undefined value is returned. If whatever object the label should point at no longer exists,
a null value is returned.

You can also fetch a STRUCTURE or FEATURE type game object from a given map position (if any).
This is a very fast operation of O(1) algorithmic complexity. Droids cannot be fetched in this
manner, since they do not have a unique placement on map tiles. Finally, you can fetch an object using
its ID, in which case you need to pass its type, owner and unique object ID. This is an
operation of O(n) algorithmic complexity. (3.2+ only)

## enumBlips(player)

Return an array containing all the non-transient radar blips that the given player
can see. This includes sensors revealed by radar detectors, as well as ECM jammers.
It does not include units going out of view.

## enumSelected()

Return an array containing all game objects currently selected by the host player. (3.2+ only)

## enumGateways()

Return an array containing all the gateways on the current map. The array contains object with the properties
x1, y1, x2 and y2. (3.2+ only)

## enumTemplates(player)

Return an array containing all the buildable templates for the given player. (3.2+ only)

## enumGroup(group)

Return an array containing all the members of a given group.

## newGroup()

Allocate a new group. Returns its numerical ID. Deprecated since 3.2 - you should now
use your own number scheme for groups.

## activateStructure(structure, [target[, ability]])

Activate a special ability on a structure. Currently only works on the lassat.
The lassat needs a target.

## findResearch(research, [player])

Return list of research items remaining to be researched for the given research item. (3.2+ only)
(Optional second argument 3.2.3+ only)

## pursueResearch(lab, research)

Start researching the first available technology on the way to the given technology.
First parameter is the structure to research in, which must be a research lab. The
second parameter is the technology to pursue, as a text string as defined in "research.json".
The second parameter may also be an array of such strings. The first technology that has
not yet been researched in that list will be pursued.

## getResearch(research[, player])

Fetch information about a given technology item, given by a string that matches
its definition in "research.json". If not found, returns null.

## enumResearch()

Returns an array of all research objects that are currently and immediately available for research.

## componentAvailable([component type,] component name)

Checks whether a given component is available to the current player. The first argument is
optional and deprecated.

## addFeature(name, x, y)

Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
Returns the created game object on success, null otherwise. (3.2+ only)

## addDroid(player, x, y, name, body, propulsion, reserved, reserved, turrets...)

Create and place a droid at the given x, y position as belonging to the given player, built with
the given components. Currently does not support placing droids in multiplayer, doing so will
cause a desync. Returns the created droid on success, otherwise returns null. Passing "" for
reserved parameters is recommended. In 3.2+ only, to create droids in off-world (campaign mission list),
pass -1 as both x and y.

## addDroidToTransporter(transporter, droid)

Load a droid, which is currently located on the campaign off-world mission list,
into a transporter, which is also currently on the campaign off-world mission list.
(3.2+ only)

## makeTemplate(player, name, body, propulsion, reserved, turrets...)

Create a template (virtual droid) with the given components. Can be useful for calculating the cost
of droids before putting them into production, for instance. Will fail and return null if template
could not possibly be built using current research. (3.2+ only)

## buildDroid(factory, name, body, propulsion, reserved, reserved, turrets...)

Start factory production of new droid with the given name, body, propulsion and turrets.
The reserved parameter should be passed **null** for now. The components can be
passed as ordinary strings, or as a list of strings. If passed as a list, the first available
component in the list will be used. The second reserved parameter used to be a droid type.
It is now unused and in 3.2+ should be passed "", while in 3.1 it should be the
droid type to be built. Returns a boolean that is true if production was started.

## enumStruct([player[, structure type[, looking player]]])

Returns an array of structure objects. If no parameters given, it will
return all of the structures for the current player. The second parameter
can be either a string with the name of the structure type as defined in
"structures.json", or a stattype as defined in ```Structure```. The
third parameter can be used to filter by visibility, the default is not
to filter.

## enumStructOffWorld([player[, structure type[, looking player]]])

Returns an array of structure objects in your base when on an off-world mission, NULL otherwise.
If no parameters given, it will return all of the structures for the current player.
The second parameter can be either a string with the name of the structure type as defined
in "structures.json", or a stattype as defined in ```Structure```.
The third parameter can be used to filter by visibility, the default is not
to filter.

## enumFeature(player[, name])

Returns an array of all features seen by player of given name, as defined in "features.json".
If player is ```ALL_PLAYERS```, it will return all features irrespective of visibility to any player. If
name is empty, it will return any feature.

## enumCargo(transport droid)

Returns an array of droid objects inside given transport. (3.2+ only)

## enumDroid([player[, droid type[, looking player]]])

Returns an array of droid objects. If no parameters given, it will
return all of the droids for the current player. The second, optional parameter
is the name of the droid type. The third parameter can be used to filter by
visibility - the default is not to filter.

## dump(string...)

Output text to a debug file. (3.2+ only)

## debug(string...)

Output text to the command line.

## pickStructLocation(droid, structure type, x, y)

Pick a location for constructing a certain type of building near some given position.
Returns an object containing "type" POSITION, and "x" and "y" values, if successful.

## structureIdle(structure)

Is given structure idle?

## removeStruct(structure)

Immediately remove the given structure from the map. Returns a boolean that is true on success.
No special effects are applied. Deprecated since 3.2.

## removeObject(game object[, special effects?])

Remove the given game object with special effects. Returns a boolean that is true on success.
A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)

## clearConsole()

Clear the console. (3.3+ only)

## console(strings...)

Print text to the player console.

## groupAddArea(group, x1, y1, x2, y2)

Add any droids inside the given area to the given group. (3.2+ only)

## groupAddDroid(group, droid)

Add given droid to given group. Deprecated since 3.2 - use groupAdd() instead.

## groupAdd(group, object)

Add given game object to the given group.

## distBetweenTwoPoints(x1, y1, x2, y2)

Return distance between two points.

## groupSize(group)

Return the number of droids currently in the given group. Note that you can use groupSizes[] instead.

## droidCanReach(droid, x, y)

Return whether or not the given droid could possibly drive to the given position. Does
not take player built blockades into account.

## propulsionCanReach(propulsion, x1, y1, x2, y2)

Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
Does not take player built blockades into account. (3.2+ only)

## terrainType(x, y)

Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
Tile types regulate which units may pass through this tile. (3.2+ only)

## orderDroid(droid, order)

Give a droid an order to do something. (3.2+ only)

## orderDroidObj(droid, order, object)

Give a droid an order to do something to something.

## orderDroidBuild(droid, order, structure type, x, y[, direction])

Give a droid an order to build something at the given position. Returns true if allowed.

## orderDroidLoc(droid, order, x, y)

Give a droid an order to do something at the given location.

## setMissionTime(time)

Set mission countdown in seconds.

## getMissionTime()

Get time remaining on mission countdown in seconds. (3.2+ only)

## setTransporterExit(x, y, player)

Set the exit position for the mission transporter. (3.2+ only)

## startTransporterEntry(x, y, player)

Set the entry position for the mission transporter, and make it start flying in
reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
The transport needs to be set up with the mission droids, and the first transport
found will be used. (3.2+ only)

## useSafetyTransport(flag)

Change if the mission transporter will fetch droids in non offworld missions
setReinforcementTime() is be used to hide it before coming back after the set time
which is handled by the campaign library in the victory data section (3.3+ only).

## restoreLimboMissionData()

Swap mission type and bring back units previously stored at the start
of the mission (see cam3-c mission). (3.3+ only).

## setReinforcementTime(time)

Set time for reinforcements to arrive. If time is negative, the reinforcement GUI
is removed and the timer stopped. Time is in seconds.
If time equals to the magic LZ_COMPROMISED_TIME constant, reinforcement GUI ticker
is set to "--:--" and reinforcements are suppressed until this function is called
again with a regular time value.

## setStructureLimits(structure type, limit[, player])

Set build limits for a structure.

## centreView(x, y)

Center the player's camera at the given position.

## hackPlayIngameAudio()

(3.3+ only)

## hackStopIngameAudio()

(3.3+ only)

## playSound(sound[, x, y, z])

Play a sound, optionally at a location.

## gameOverMessage(won, showBackDrop, showOutro)

End game in victory or defeat.

## completeResearch(research[, player [, forceResearch]])

Finish a research for the given player.
forceResearch will allow a research topic to be researched again. 3.3+

## enableResearch(research[, player])

Enable a research for the given player, allowing it to be researched.

## extraPowerTime(time, player)

Increase a player's power as if that player had power income equal to current income
over the given amount of extra time. (3.2+ only)

## setPower(power[, player])

Set a player's power directly. (Do not use this in an AI script.)

## setPowerModifier(power[, player])

Set a player's power modifier percentage. (Do not use this in an AI script.) (3.2+ only)

## setPowerStorageMaximum(maximum[, player])

Set a player's power storage maximum. (Do not use this in an AI script.) (3.2+ only)

## enableStructure(structure type[, player])

The given structure type is made available to the given player. It will appear in the
player's build list.

## setTutorialMode(bool)

Sets a number of restrictions appropriate for tutorial if set to true.

## setMiniMap(bool)

Turns visible minimap on or off in the GUI.

## setDesign(bool)

Whether to allow player to design stuff.

## enableTemplate(template name)

Enable a specific template (even if design is disabled).

## removeTemplate(template name)

Remove a template.

## setReticuleButton(id, filename, filenameHigh, tooltip, callback)

Add reticule button. id is which button to change, where zero is zero is the middle button, then going clockwise from the
uppermost button. filename is button graphics and filenameHigh is for highlighting. The tooltip is the text you see when
you mouse over the button. Finally, the callback is which scripting function to call. Hide and show the user interface
for such changes to take effect. (3.2+ only)

## showReticuleWidget(id)

Open the reticule menu widget. (3.3+ only)

## setReticuleFlash(id, flash)

Set reticule flash on or off. (3.2.3+ only)

## showInterface()

Show user interface. (3.2+ only)

## hideInterface(button type)

Hide user interface. (3.2+ only)

## removeReticuleButton(button type)

Remove reticule button. DO NOT USE FOR ANYTHING.

## applyLimitSet()

Mix user set limits with script set limits and defaults.

## enableComponent(component, player)

The given component is made available for research for the given player.

## makeComponentAvailable(component, player)

The given component is made available to the given player. This means the player can
actually build designs with it.

## allianceExistsBetween(player, player)

Returns true if an alliance exists between the two players, or they are the same player.

## _(string)

Mark string for translation.

## playerPower(player)

Return amount of power held by the given player.

## queuedPower(player)

Return amount of power queued up for production by the given player. (3.2+ only)

## isStructureAvailable(structure type[, player])

Returns true if given structure can be built. It checks both research and unit limits.

## isVTOL(droid)

Returns true if given droid is a VTOL (not including transports).

## hackGetObj(type, player, id)

Function to find and return a game object of DROID, FEATURE or STRUCTURE types, if it exists.
Otherwise, it will return null. This function is deprecated by getObject(). (3.2+ only)

## hackChangeMe(player)

Change the 'me' who owns this script to the given player. This needs to be run
first in ```eventGameInit``` to make sure things do not get out of control.

## receiveAllEvents(bool)

Make the current script receive all events, even those not meant for 'me'. (3.2+ only)

## hackAssert(condition, message...)

Function to perform unit testing. It will throw a script error and a game assert. (3.2+ only)

## objFromId(fake game object)

Broken function meant to make porting from the old scripting system easier. Do not use for new code.
Instead, use labels.

## setDroidExperience(droid, experience)

Set the amount of experience a droid has. Experience is read using floating point precision.

## donateObject(object, to)

Donate a game object (restricted to droids before 3.2.3) to another player. Returns true if
donation was successful. May return false if this donation would push the receiving player
over unit limits. (3.2+ only)

## donatePower(amount, to)

Donate power to another player. Returns true. (3.2+ only)

## safeDest(player, x, y)

Returns true if given player is safe from hostile fire at the given location, to
the best of that player's map knowledge. Does not work in campaign at the moment.

## addStructure(structure type, player, x, y)

Create a structure on the given position. Returns the structure on success, null otherwise.

## getStructureLimit(structure type[, player])

Returns build limits for a structure.

## countStruct(structure type[, player])

Count the number of structures of a given type.
The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.

## countDroid([droid type[, player]])

Count the number of droids that a given player has. Droid type must be either
DROID_ANY, DROID_COMMAND or DROID_CONSTRUCT.
The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.

## setNoGoArea(x1, y1, x2, y2, player)

Creates an area on the map on which nothing can be built. If player is zero,
then landing lights are placed. If player is -1, then a limbo landing zone
is created and limbo droids placed.

## setScrollLimits(x1, y1, x2, y2)

Limit the scrollable area of the map to the given rectangle. (3.2+ only)

## getScrollLimits()

Get the limits of the scrollable area of the map as an area object. (3.2+ only)

## loadLevel(level name)

Load the level with the given name.

## enumRange(x, y, range[, filter[, seen]])

Returns an array of game objects seen within range of given position that passes the optional filter
which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
returned; by default only visible objects are returned. Calling this function is much faster than
iterating over all game objects using other enum functions. (3.2+ only)

## enumArea(<x1, y1, x2, y2 | label>[, filter[, seen]])

Returns an array of game objects seen within the given area that passes the optional filter
which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
returned; by default only visible objects are returned. The label can either be actual
positions or a label to an AREA. Calling this function is much faster than iterating over all
game objects using other enum functions. (3.2+ only)

## addBeacon(x, y, target player[, message])

Send a beacon message to target player. Target may also be ```ALLIES```.
Message is currently unused. Returns a boolean that is true on success. (3.2+ only)

## removeBeacon(target player)

Remove a beacon message sent to target player. Target may also be ```ALLIES```.
Returns a boolean that is true on success. (3.2+ only)

## chat(target player, message)

Send a message to target player. Target may also be ```ALL_PLAYERS``` or ```ALLIES```.
Returns a boolean that is true on success. (3.2+ only)

## setAlliance(player1, player2, value)

Set alliance status between two players to either true or false. (3.2+ only)

## sendAllianceRequest(player)

Send an alliance request to a player. (3.3+ only)

## setAssemblyPoint(structure, x, y)

Set the assembly point droids go to when built for the specified structure. (3.2+ only)

## hackNetOff()

Turn off network transmissions. FIXME - find a better way.

## hackNetOn()

Turn on network transmissions. FIXME - find a better way.

## getDroidProduction(factory)

Return droid in production in given factory. Note that this droid is fully
virtual, and should never be passed anywhere. (3.2+ only)

## getDroidLimit([player[, unit type]])

Return maximum number of droids that this player can produce. This limit is usually
fixed throughout a game and the same for all players. If no arguments are passed,
returns general unit limit for the current player. If a second, unit type argument
is passed, the limit for this unit type is returned, which may be different from
the general unit limit (eg for commanders and construction droids). (3.2+ only)

## getExperienceModifier(player)

Get the percentage of experience this player droids are going to gain. (3.2+ only)

## setExperienceModifier(player, percent)

Set the percentage of experience this player droids are going to gain. (3.2+ only)

## setDroidLimit(player, value[, droid type])

Set the maximum number of droids that this player can produce. If a third
parameter is added, this is the droid type to limit. It can be DROID_ANY
for droids in general, DROID_CONSTRUCT for constructors, or DROID_COMMAND
for commanders. (3.2+ only)

## setCommanderLimit(player, value)

Set the maximum number of commanders that this player can produce.
THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)

## setConstructorLimit(player, value)

Set the maximum number of constructors that this player can produce.
THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)

## hackAddMessage(message, type, player, immediate)

See wzscript docs for info, to the extent any exist. (3.2+ only)

## hackRemoveMessage(message, type, player)

See wzscript docs for info, to the extent any exist. (3.2+ only)

## setSunPosition(x, y, z)

Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)

## setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)

Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)

## setWeather(weather type)

Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)

## setSky(texture file, wind speed, skybox scale)

Change the skybox. (3.2+ only)

## hackDoNotSave(name)

Do not save the given global given by name to savegames. Must be
done again each time game is loaded, since this too is not saved.

## hackMarkTiles([label | x, y[, x2, y2]])

Mark the given tile(s) on the map. Either give a POSITION or AREA label,
or a tile x, y position, or four positions for a square area. If no parameter
is given, all marked tiles are cleared. (3.2+ only)

## cameraSlide(x, y)

Slide the camera over to the given position on the map. (3.2+ only)

## cameraZoom(z, speed)

Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)

## cameraTrack(droid)

Make the camera follow the given droid object around. Pass in a null object to stop. (3.2+ only)

## setHealth(object, health)

Change the health of the given game object, in percentage. Does not take care of network sync, so for multiplayer games,
needs wrapping in a syncRequest. (3.2.3+ only.)

## setObjectFlag(object, flag, value)

Set or unset an object flag on a given game object. Does not take care of network sync, so for multiplayer games,
needs wrapping in a syncRequest. (3.3+ only.)
Recognized object flags: OBJECT_FLAG_UNSELECTABLE - makes object unavailable for selection from player UI.

## addSpotter(x, y, player, range, type, expiry)

Add an invisible viewer at a given position for given player that shows map in given range. ```type```
is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
by ECM jammers. ```expiry```, if non-zero, is the game time at which the spotter shall automatically be
removed. The function returns a unique ID that can be used to remove the spotter with ```removeSpotter```. (3.2+ only)

## removeSpotter(id)

Remove a spotter given its unique ID. (3.2+ only)

## syncRandom(limit)

Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
run on all network peers in the same game frame. If it is called on just one peer (such as would be
the case for AIs, for instance), then game sync will break. (3.2+ only)

## syncRequest(req_id, x, y[, obj[, obj2]])

Generate a synchronized event request that is sent over the network to all clients and executed simultaneously.
Must be caught in an eventSyncRequest() function. All sync requests must be validated when received, and always
take care only to define sync requests that can be validated against cheating. (3.2+ only)

## replaceTexture(old_filename, new_filename)

Replace one texture with another. This can be used to for example give buildings on a specific tileset different
looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)

## fireWeaponAtLoc(x, y, weapon_name)

Fires a weapon at the given coordinates (3.3+ only).

## changePlayerColour(player, colour)

Change a player's colour slot. The current player colour can be read from the ```playerData``` array. There are as many
colour slots as the maximum number of players. (3.2.3+ only)

## getMultiTechLevel()

Returns the current multiplayer tech level. (3.3+ only)

## setCampaignNumber(num)

Set the campaign number. (3.3+ only)

## getMissionType()

Return the current mission type. (3.3+ only)
