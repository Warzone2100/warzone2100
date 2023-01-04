# Functions

This section describes functions that can be called from scripts to make
things happen in the game (usually called our script 'API').

## profile(functionName[, arguments])

Calls a function with given arguments, measures time it took to evaluate the function,
and adds this time to performance monitor statistics. Transparently returns the
function's return value. The function to run is the first parameter, and it
_must be quoted_. (3.2+ only)

## include(filePath)

Includes another source code file at this point. You should generally only specify the filename,
not try to specify its path, here.
However, *if* you specify sub-paths / sub-folders, the path separator should **always** be forward-slash ("/").

## includeJSON(filePath)

Reads a JSON file and returns an object. You should generally only specify the filename,
However, *if* you specify sub-paths / sub-folders, the path separator should **always** be forward-slash ("/").

## setTimer(functionName, milliseconds[, object])

Set a function to run repeated at some given time interval. The function to run
is the first parameter, and it _must be quoted_, otherwise the function will
be inlined. The second parameter is the interval, in milliseconds. A third, optional
parameter can be a **game object** to pass to the timer function. If the **game object**
dies, the timer stops running. The minimum number of milliseconds is 100, but such
fast timers are strongly discouraged as they may deteriorate the game performance.

```js
function conDroids()
{
  ... do stuff ...
}
// call conDroids every 4 seconds
setTimer("conDroids", 4000);
```

## removeTimer(functionName)

Removes an existing timer. The first parameter is the function timer to remove,
and its name _must be quoted_.

## queue(functionName[, milliseconds[, object]])

Queues up a function to run at a later game frame. This is useful to prevent
stuttering during the game, which can happen if too much script processing is
done at once.  The function to run is the first parameter, and it
_must be quoted_, otherwise the function will be inlined.
The second parameter is the delay in milliseconds, if it is omitted or 0,
the function will be run at a later frame.  A third optional
parameter can be a **game object** to pass to the queued function. If the **game object**
dies before the queued call runs, nothing happens.

## namespace(prefix)

Registers a new event namespace. All events can now have this prefix. This is useful for
code libraries, to implement event that do not conflict with events in main code. This
function should be called from global; do not (for hopefully obvious reasons) put it
inside an event.

## debugGetCallerFuncName()

Returns the function name of the caller of the current context as a string (if available).
ex.
```js
function funcA() {
  const callerFuncName = debugGetCallerFuncName();
  debug(callerFuncName);
}
function funcB() {
  funcA();
}
funcB();
```
Will output: "funcB"
Useful for debug logging.

## enumTemplates(player)

Return an array containing all the buildable templates for the given player. (3.2+ only)

## removeReticuleButton(buttonId)

Remove reticule button. DO NOT USE FOR ANYTHING.

## removeBeacon(playerFilter)

Remove a beacon message sent to target player. Target may also be ```ALLIES```.
Returns a boolean that is true on success. (3.2+ only)

## resetLabel(labelName[, playerFilter])

Reset the trigger on an label. Next time a unit enters the area, it will trigger
an area event. Next time an object or a group is seen, it will trigger a seen event.
Optionally add a filter on it in the second parameter, which can
be a specific player to watch for, or ```ALL_PLAYERS``` by default.
This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)

## resetArea(labelName[, playerFilter])

Reset the trigger on an area. Next time a unit enters the area, it will trigger
an area event. Optionally add a filter on it in the second parameter, which can
be a specific player to watch for, or ```ALL_PLAYERS``` by default.
This is a fast operation of O(log n) algorithmic complexity. DEPRECATED - use resetLabel instead. (3.2+ only)

## enumLabels([filterLabelType])

Returns a string list of labels that exist for this map. The optional filter
parameter can be used to only return labels of one specific type. (3.2+ only)

## addLabel(object, label[, triggered])

Add a label to a game object. If there already is a label by that name, it is overwritten.
This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
Can optionally specify an initial "triggered" value for the label. (3.4+ only)

## removeLabel(label)

Remove a label from the game. Returns the number of labels removed, which should normally be
either 1 (label found) or 0 (label not found). (3.2+ only)

## getLabel(object)

Get a label string belonging to a game object. If the object has multiple labels, only the first
label found will be returned. If the object has no labels, undefined is returned.
This is a relatively slow operation of O(n) algorithmic complexity. (3.2+ only)

## getObject(label | x, y | type, player, id)

Fetch something denoted by a label, a map position or its object ID. A label refers to an area,
a position or a **game object** on the map defined using the map editor and stored
together with the map. In this case, the only argument is a text label. The function
returns an object that has a type variable defining what it is (in case this is
unclear). This type will be one of ```DROID```, ```STRUCTURE```, ```FEATURE```, ```AREA```, ```GROUP``` or ```POSITION```.
The ```AREA``` has defined 'x', 'y', 'x2', and 'y2', while ```POSITION``` has only defined 'x' and 'y'.
The ```GROUP``` type has defined 'type' and 'id' of the group, which can be passed to enumGroup().
This is a fast operation of O(log n) algorithmic complexity. If the label is not found, an
undefined value is returned. If whatever object the label should point at no longer exists,
a null value is returned.

You can also fetch a ```STRUCTURE``` or ```FEATURE``` type game object from a given map position (if any).
This is a very fast operation of O(1) algorithmic complexity. Droids cannot be fetched in this
manner, since they do not have a unique placement on map tiles. Finally, you can fetch an object using
its ID, in which case you need to pass its type, owner and unique object ID. This is an
operation of O(n) algorithmic complexity. (3.2+ only)

## enumArea(<x1, y1, x2, y2 | label>[, playerFilter[, seen]])

Returns an array of game objects seen within the given area that passes the optional filter
which can be one of a player index, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```. By default, filter is
```ALL_PLAYERS```. Finally an optional parameter can specify whether only visible objects should be
returned; by default only visible objects are returned. The label can either be actual
positions or a label to an ```AREA```. Calling this function is much faster than iterating over all
game objects using other enum functions. (3.2+ only)

## enumGroup(groupId)

Return an array containing all the members of a given group.

## newGroup()

Allocate a new group. Returns its numerical ID. Deprecated since 3.2 - you should now
use your own number scheme for groups.

## groupAddArea(groupId, x1, y1, x2, y2)

Add any droids inside the given area to the given group. (3.2+ only)

## groupAddDroid(groupId, droid)

Add given droid to given group. Deprecated since 3.2 - use groupAdd() instead.

## groupAdd(groupId, object)

Add given game object to the given group.

## groupSize(groupId)

Return the number of droids currently in the given group. Note that you can use groupSizes[] instead.

## _(string)

Mark string for translation.

## syncRandom(limit)

Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
run on all network peers in the same game frame. If it is called on just one peer (such as would be
the case for AIs, for instance), then game sync will break. (3.2+ only)

## setAlliance(player1, player2, areAllies)

Set alliance status between two players to either true or false. (3.2+ only)

## sendAllianceRequest(player)

Send an alliance request to a player. (3.3+ only)

## orderDroid(droid, order)

Give a droid an order to do something. (3.2+ only)

## orderDroidBuild(droid, order, structureName, x, y[, direction])

Give a droid an order to build something at the given position. Returns true if allowed.

## setAssemblyPoint(structure, x, y)

Set the assembly point droids go to when built for the specified structure. (3.2+ only)

## setSunPosition(x, y, z)

Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)

## setSunIntensity(ambient_r, ambient_g, ambient_b, diffuse_r, diffuse_g, diffuse_b, specular_r, specular_g, specular_b)

Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)

## setFogColour(r, g, b)

Set the colour of the fog (4.2.5+ only)

## setWeather(weatherType)

Set the current weather. This should be one of ```WEATHER_RAIN```, ```WEATHER_SNOW``` or ```WEATHER_CLEAR```. (3.2+ only)

## setSky(textureFilename, windSpeed, scale)

Change the skybox. (3.2+ only)

## cameraSlide(x, y)

Slide the camera over to the given position on the map. (3.2+ only)

## cameraZoom(viewDistance, speed)

Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)

## cameraTrack([droid])

Make the camera follow the given droid object around. Pass in a null object to stop. (3.2+ only)

## addSpotter(x, y, player, range, radar, expiry)

Add an invisible viewer at a given position for given player that shows map in given range. ```radar```
is false for vision reveal, or true for radar reveal. The difference is that a radar reveal can be obstructed
by ECM jammers. ```expiry```, if non-zero, is the game time at which the spotter shall automatically be
removed. The function returns a unique ID that can be used to remove the spotter with ```removeSpotter```. (3.2+ only)

## removeSpotter(spotterId)

Remove a spotter given its unique ID. (3.2+ only)

## syncRequest(req_id, x, y[, object[, object2]])

Generate a synchronized event request that is sent over the network to all clients and executed simultaneously.
Must be caught in an eventSyncRequest() function. All sync requests must be validated when received, and always
take care only to define sync requests that can be validated against cheating. (3.2+ only)

## replaceTexture(oldFilename, newFilename)

Replace one texture with another. This can be used to for example give buildings on a specific tileset different
looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)

## changePlayerColour(player, colour)

Change a player's colour slot. The current player colour can be read from the ```playerData``` array. There are as many
colour slots as the maximum number of players. (3.2.3+ only)

## setHealth(object, health)

Change the health of the given game object, in percentage. Does not take care of network sync, so for multiplayer games,
needs wrapping in a syncRequest. (3.2.3+ only.)

## useSafetyTransport(flag)

Change if the mission transporter will fetch droids in non offworld missions
setReinforcementTime() is be used to hide it before coming back after the set time
which is handled by the campaign library in the victory data section (3.3+ only).

## restoreLimboMissionData()

Swap mission type and bring back units previously stored at the start
of the mission (see cam3-c mission). (3.3+ only).

## getMultiTechLevel()

Returns the current multiplayer tech level. (3.3+ only)

## setCampaignNumber(campaignNumber)

Set the campaign number. (3.3+ only)

## getMissionType()

Return the current mission type. (3.3+ only)

## getRevealStatus()

Return the current fog reveal status. (3.3+ only)

## setRevealStatus(status)

Set the fog reveal status. (3.3+ only)

## autoSave()

Perform automatic save

## hackNetOff()

Turn off network transmissions. FIXME - find a better way.

## hackNetOn()

Turn on network transmissions. FIXME - find a better way.

## hackAddMessage(message, messageType, player, immediate)

See wzscript docs for info, to the extent any exist. (3.2+ only)

## hackRemoveMessage(message, messageType, player)

See wzscript docs for info, to the extent any exist. (3.2+ only)

## hackGetObj(objectType, player, id)

Function to find and return a game object of ```DROID```, ```FEATURE``` or ```STRUCTURE``` types, if it exists.
Otherwise, it will return null. This function is deprecated by getObject(). (3.2+ only)

## hackAssert(condition, message...)

Function to perform unit testing. It will throw a script error and a game assert. (3.2+ only)

## receiveAllEvents([enabled])

Make the current script receive all events, even those not meant for 'me'. (3.2+ only)

## hackDoNotSave(name)

Do not save the given global given by name to savegames. Must be
done again each time game is loaded, since this too is not saved.

## hackPlayIngameAudio()

(3.3+ only)

## hackStopIngameAudio()

Stop the in-game music. (3.3+ only)
This should be called from the eventStartLevel() event (or later).
Currently only used from the tutorial.

## hackMarkTiles([label | x, y[, x2, y2]])

Mark the given tile(s) on the map. Either give a ```POSITION``` or ```AREA``` label,
or a tile x, y position, or four positions for a square area. If no parameter
is given, all marked tiles are cleared. (3.2+ only)

## dump(string...)

Output text to a debug file. (3.2+ only)

## debug(string...)

Output text to the command line.

## console(strings...)

Print text to the player console.

## clearConsole()

Clear the console. (3.3+ only)

## structureIdle(structure)

Is given structure idle?

## enumStruct([player[, structureType[, playerFilter]]])

Returns an array of structure objects. If no parameters given, it will
return all of the structures for the current player. The second parameter
can be either a string with the name of the structure type as defined in
"structures.json", or a stattype as defined in ```Structure```. The
third parameter can be used to filter by visibility, the default is not
to filter.

## enumStructOffWorld([player[, structureType[, playerFilter]]])

Returns an array of structure objects in your base when on an off-world mission, NULL otherwise.
If no parameters given, it will return all of the structures for the current player.
The second parameter can be either a string with the name of the structure type as defined
in "structures.json", or a stattype as defined in ```Structure```.
The third parameter can be used to filter by visibility, the default is not
to filter.

## enumDroid([player[, droidType[, playerFilter]]])

Returns an array of droid objects. If no parameters given, it will
return all of the droids for the current player. The second, optional parameter
is the name of the droid type. The third parameter can be used to filter by
visibility - the default is not to filter.

## enumFeature(playerFilter[, featureName])

Returns an array of all features seen by player of given name, as defined in "features.json".
If player is ```ALL_PLAYERS```, it will return all features irrespective of visibility to any player. If
name is empty, it will return any feature.

## enumBlips(player)

Return an array containing all the non-transient radar blips that the given player
can see. This includes sensors revealed by radar detectors, as well as ECM jammers.
It does not include units going out of view.

## enumSelected()

Return an array containing all game objects currently selected by the host player. (3.2+ only)

## enumGateways()

Return an array containing all the gateways on the current map. The array contains object with the properties
x1, y1, x2 and y2. (3.2+ only)

## getResearch(researchName[, player])

Fetch information about a given technology item, given by a string that matches
its definition in "research.json". If not found, returns null.

## enumResearch()

Returns an array of all research objects that are currently and immediately available for research.

## enumRange(x, y, range[, playerFilter[, seen]])

Returns an array of game objects seen within range of given position that passes the optional filter
which can be one of a player index, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```. By default, filter is
```ALL_PLAYERS```. Finally an optional parameter can specify whether only visible objects should be
returned; by default only visible objects are returned. Calling this function is much faster than
iterating over all game objects using other enum functions. (3.2+ only)

## pursueResearch(labStructure, research)

Start researching the first available technology on the way to the given technology.
First parameter is the structure to research in, which must be a research lab. The
second parameter is the technology to pursue, as a text string as defined in "research.json".
The second parameter may also be an array of such strings. The first technology that has
not yet been researched in that list will be pursued.

## findResearch(researchName[, player])

Return list of research items remaining to be researched for the given research item. (3.2+ only)
(Optional second argument 3.2.3+ only)

## distBetweenTwoPoints(x1, y1, x2, y2)

Return distance between two points.

## orderDroidLoc(droid, order, x, y)

Give a droid an order to do something at the given location.

## playerPower(player)

Return amount of power held by the given player.

## queuedPower(player)

Return amount of power queued up for production by the given player. (3.2+ only)

## isStructureAvailable(structureName[, player])

Returns true if given structure can be built. It checks both research and unit limits.

## pickStructLocation(droid, structureName, x, y[, maxBlockingTiles])

Pick a location for constructing a certain type of building near some given position.
Returns an object containing "type" ```POSITION```, and "x" and "y" values, if successful.

## droidCanReach(droid, x, y)

Return whether or not the given droid could possibly drive to the given position. Does
not take player built blockades into account.

## propulsionCanReach(propulsionName, x1, y1, x2, y2)

Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
Does not take player built blockades into account. (3.2+ only)

## terrainType(x, y)

Returns tile type of a given map tile, such as ```TER_WATER``` for water tiles or ```TER_CLIFFFACE``` for cliffs.
Tile types regulate which units may pass through this tile. (3.2+ only)

## tileIsBurning(x, y)

Returns whether the given map tile is burning. (3.5+ only)

## orderDroidObj(droid, order, object)

Give a droid an order to do something to something.

## buildDroid(factory, templateName, body, propulsion, reserved, reserved, turrets...)

Start factory production of new droid with the given name, body, propulsion and turrets.
The reserved parameter should be passed **null** for now. The components can be
passed as ordinary strings, or as a list of strings. If passed as a list, the first available
component in the list will be used. The second reserved parameter used to be a droid type.
It is now unused and in 3.2+ should be passed "", while in 3.1 it should be the
droid type to be built. Returns a boolean that is true if production was started.

## addDroid(player, x, y, templateName, body, propulsion, reserved, reserved, turrets...)

Create and place a droid at the given x, y position as belonging to the given player, built with
the given components. Currently does not support placing droids in multiplayer, doing so will
cause a desync. Returns the created droid on success, otherwise returns null. Passing "" for
reserved parameters is recommended. In 3.2+ only, to create droids in off-world (campaign mission list),
pass -1 as both x and y.

## makeTemplate(player, templateName, body, propulsion, reserved, turrets...)

Create a template (virtual droid) with the given components. Can be useful for calculating the cost
of droids before putting them into production, for instance. Will fail and return null if template
could not possibly be built using current research. (3.2+ only)

## addDroidToTransporter(transporter, droid)

Load a droid, which is currently located on the campaign off-world mission list,
into a transporter, which is also currently on the campaign off-world mission list.
(3.2+ only)

## addFeature(featureName, x, y)

Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
Returns the created game object on success, null otherwise. (3.2+ only)

## componentAvailable([componentType, ]componentName)

Checks whether a given component is available to the current player. The first argument is
optional and deprecated.

## isVTOL(droid)

Returns true if given droid is a VTOL (not including transports).

## safeDest(player, x, y)

Returns true if given player is safe from hostile fire at the given location, to
the best of that player's map knowledge. Does not work in campaign at the moment.

## activateStructure(structure[, target])

Activate a special ability on a structure. Currently only works on the lassat.
The lassat needs a target.

## chat(playerFilter, message)

Send a message to target player. Target may also be ```ALL_PLAYERS``` or ```ALLIES```.
Returns a boolean that is true on success. (3.2+ only)

## addBeacon(x, y, playerFilter[, message])

Send a beacon message to target player. Target may also be ```ALLIES```.
Message is currently unused. Returns a boolean that is true on success. (3.2+ only)

## removeBeacon(playerFilter)

Remove a beacon message sent to target player. Target may also be ```ALLIES```.
Returns a boolean that is true on success. (3.2+ only)

## getDroidProduction(factory)

Return droid in production in given factory. Note that this droid is fully
virtual, and should never be passed anywhere. (3.2+ only)

## getDroidLimit([player[, droidType]])

Return maximum number of droids that this player can produce. This limit is usually
fixed throughout a game and the same for all players. If no arguments are passed,
returns general droid limit for the current player. If a second, droid type argument
is passed, the limit for this droid type is returned, which may be different from
the general droid limit (eg for commanders and construction droids). (3.2+ only)

## getExperienceModifier(player)

Get the percentage of experience this player droids are going to gain. (3.2+ only)

## setDroidLimit(player, maxNumber[, droidType])

Set the maximum number of droids that this player can produce. If a third
parameter is added, this is the droid type to limit. It can be DROID_ANY
for droids in general, ```DROID_CONSTRUCT``` for constructors, or ```DROID_COMMAND```
for commanders. (3.2+ only)

## setCommanderLimit(player, maxNumber)

Set the maximum number of commanders that this player can produce.
THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)

## setConstructorLimit(player, maxNumber)

Set the maximum number of constructors that this player can produce.
THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)

## setExperienceModifier(player, percent)

Set the percentage of experience this player droids are going to gain. (3.2+ only)

## enumCargo(transporterDroid)

Returns an array of droid objects inside given transport. (3.2+ only)

## isSpectator(player)

Returns whether a particular player is a spectator. (4.2+ only)
Can pass -1 as player to get the spectator status of the client running the script. (Useful for the "rules" scripts.)

## getWeaponInfo(weaponName)

Return information about a particular weapon type. DEPRECATED - query the Stats object instead. (3.2+ only)

## centreView(x, y)

Center the player's camera at the given position.

## playSound(sound[, x, y, z])

Play a sound, optionally at a location.

## gameOverMessage(gameWon[, showBackDrop[, showOutro]])

End game in victory or defeat.

## setStructureLimits(structureName, limit[, player])

Set build limits for a structure.

## applyLimitSet()

Mix user set limits with script set limits and defaults.

## setMissionTime(time)

Set mission countdown in seconds.

## getMissionTime()

Get time remaining on mission countdown in seconds. (3.2+ only)

## setReinforcementTime(time)

Set time for reinforcements to arrive. If time is negative, the reinforcement GUI
is removed and the timer stopped. Time is in seconds.
If time equals to the magic ```LZ_COMPROMISED_TIME``` constant, reinforcement GUI ticker
is set to "--:--" and reinforcements are suppressed until this function is called
again with a regular time value.

## completeResearch(research[, player[, forceResearch]])

Finish a research for the given player.
forceResearch will allow a research topic to be researched again. 3.3+

## completeAllResearch([player])

Finish all researches for the given player.

## enableResearch(researchName[, player])

Enable a research for the given player, allowing it to be researched.

## setPower(power[, player])

Set a player's power directly. (Do not use this in an AI script.)

## setPowerModifier(powerModifier[, player])

Set a player's power modifier percentage. (Do not use this in an AI script.) (3.2+ only)

## setPowerStorageMaximum(powerMaximum[, player])

Set a player's power storage maximum. (Do not use this in an AI script.) (3.2+ only)

## extraPowerTime(time[, player])

Increase a player's power as if that player had power income equal to current income
over the given amount of extra time. (3.2+ only)

## setTutorialMode(enableTutorialMode)

Sets a number of restrictions appropriate for tutorial if set to true.

## setDesign(allowDesignValue)

Whether to allow player to design stuff.

## enableTemplate(templateName)

Enable a specific template (even if design is disabled).

## removeTemplate(templateName)

Remove a template.

## setMiniMap(visible)

Turns visible minimap on or off in the GUI.

## setReticuleButton(buttonId, tooltip, filename, filenameDown[, callback])

Add reticule button. buttonId is which button to change, where zero is zero is the middle button, then going clockwise from
the uppermost button. filename is button graphics and filenameDown is for highlighting. The tooltip is the text you see when
you mouse over the button. Finally, the callback is which scripting function to call. Hide and show the user interface
for such changes to take effect. (3.2+ only)

## setReticuleFlash(buttonId, flash)

Set reticule flash on or off. (3.2.3+ only)

## showReticuleWidget(buttonId)

Open the reticule menu widget. (3.3+ only)

## showInterface()

Show user interface. (3.2+ only)

## hideInterface()

Hide user interface. (3.2+ only)

## enableStructure(structureName[, player])

The given structure type is made available to the given player. It will appear in the
player's build list.

## enableComponent(componentName, player)

The given component is made available for research for the given player.

## makeComponentAvailable(componentName, player)

The given component is made available to the given player. This means the player can
actually build designs with it.

## allianceExistsBetween(player1, player2)

Returns true if an alliance exists between the two players, or they are the same player.

## removeStruct(structure)

Immediately remove the given structure from the map. Returns a boolean that is true on success.
No special effects are applied. Deprecated since 3.2. Use `removeObject` instead.

## removeObject(gameObject[, sfx])

Remove the given game object with special effects. Returns a boolean that is true on success.
A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)

## setScrollLimits(x1, y1, x2, y2)

Limit the scrollable area of the map to the given rectangle. (3.2+ only)

## getScrollLimits()

Get the limits of the scrollable area of the map as an area object. (3.2+ only)

## addStructure(structureName, player, x, y)

Create a structure on the given position. Returns the structure on success, null otherwise.
Position uses world coordinates, if you want use position based on Map Tiles, then
use as addStructure(structureName, players, x*128, y*128)

## getStructureLimit(structureName[, player])

Returns build limits for a structure.

## countStruct(structureName[, playerFilter])

Count the number of structures of a given type.
The playerFilter parameter can be a specific player, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```.

## countDroid([droidType[, playerFilter]])

Count the number of droids that a given player has. Droid type must be either
```DROID_ANY```, ```DROID_COMMAND``` or ```DROID_CONSTRUCT```.
The playerFilter parameter can be a specific player, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```.

## loadLevel(levelName)

Load the level with the given name.

## setDroidExperience(droid, experience)

Set the amount of experience a droid has. Experience is read using floating point precision.

## donateObject(object, player)

Donate a game object (restricted to droids before 3.2.3) to another player. Returns true if
donation was successful. May return false if this donation would push the receiving player
over unit limits. (3.2+ only)

## donatePower(amount, player)

Donate power to another player. Returns true. (3.2+ only)

## setNoGoArea(x1, y1, x2, y2, playerFilter)

Creates an area on the map on which nothing can be built. If player is zero,
then landing lights are placed. If player is ```ALL_PLAYERS```, then a limbo landing zone
is created and limbo droids placed.

## startTransporterEntry(x, y, player)

Set the entry position for the mission transporter, and make it start flying in
reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
The transport needs to be set up with the mission droids, and the first transport
found will be used. (3.2+ only)

## setTransporterExit(x, y, player)

Set the exit position for the mission transporter. (3.2+ only)

## setObjectFlag(object, flag, flagValue)

Set or unset an object flag on a given game object. Does not take care of network sync, so for multiplayer games,
needs wrapping in a syncRequest. (3.3+ only.)
Recognized object flags: ```OBJECT_FLAG_UNSELECTABLE``` - makes object unavailable for selection from player UI.

## fireWeaponAtLoc(weaponName, x, y[, player])

Fires a weapon at the given coordinates (3.3+ only). The player is who owns the projectile.
Please use fireWeaponAtObj() to damage objects as multiplayer and campaign
may have different friendly fire logic for a few weapons (like the lassat).

## fireWeaponAtObj(weapon, gameObject[, player])

Fires a weapon at a game object (3.3+ only). The player is who owns the projectile.

## transformPlayerToSpectator(player)

Transform a player to a spectator. (4.2+ only)
This is a one-time transformation, destroys the player's HQ and all of their remaining units, and must occur deterministically on all clients.
