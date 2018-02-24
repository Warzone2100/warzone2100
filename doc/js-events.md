# Events

This section describes event callbacks (or 'events' for short) that are
called from the game when something specific happens. Which scripts
receive them is usually filtered by player. Call ```receiveAllEvents(true)```
to start receiving all events unfiltered.

## eventGameInit()

An event that is run once as the game is initialized. Not all game state may have been
properly initialized by this time, so use this only to initialize script state.

## eventStartLevel()

An event that is run once the game has started and all game data has been loaded.

## eventMissionTimeout()

An event that is run when the mission timer has run out.

## eventVideoDone()

An event that is run when a video show stopped playing.

## eventGameLoaded()

An event that is run when game is loaded from a saved game. There is usually no need to use this event.

## eventGameSaving()

An event that is run before game is saved. There is usually no need to use this event.

## eventGameSaved()

An event that is run after game is saved. There is usually no need to use this event.

## eventDeliveryPointMoving()

An event that is run when the current player starts to move a delivery point.

## eventDeliveryPointMoved()

An event that is run after the current player has moved a delivery point.

## eventTransporterLaunch(transport)

An event that is run when the mission transporter has been ordered to fly off.

## eventTransporterArrived(transport)

An event that is run when the mission transporter has arrived at the map edge with reinforcements.

## eventTransporterExit(transport)

An event that is run when the mission transporter has left the map.

## eventTransporterDone(transport)

An event that is run when the mission transporter has no more reinforcements to deliver.

## eventTransporterLanded(transport)

An event that is run when the mission transporter has landed with reinforcements.

## eventDesignBody()

An event that is run when current user picks a body in the design menu.

## eventDesignPropulsion()

An event that is run when current user picks a propulsion in the design menu.

## eventDesignWeapon()

An event that is run when current user picks a weapon in the design menu.

## eventDesignCommand()

An event that is run when current user picks a command turret in the design menu.

## eventDesignSystem()

An event that is run when current user picks a system other than command turret in the design menu.

## eventDesignQuit()

An event that is run when current user leaves the design menu.

## eventMenuBuildSelected()

An event that is run when current user picks something new in the build menu.

## eventMenuBuild()

An event that is run when current user opens the build menu.

## eventMenuResearch()

An event that is run when current user opens the research menu.

## eventMenuManufacture()
An event that is run when current user opens the manufacture menu.

## eventPlayerLeft(player index)

An event that is run after a player has left the game.

## eventCheatMode(entered)

Game entered or left cheat/debug mode.
The entered parameter is true if cheat mode entered, false otherwise.

## eventDroidIdle(droid)

A droid should be given new orders.

## eventDroidBuilt(droid[, structure])

An event that is run every time a droid is built. The structure parameter is set
if the droid was produced in a factory. It is not triggered for droid theft or
gift (check ```eventObjectTransfer``` for that).

## eventStructureBuilt(structure[, droid])

An event that is run every time a structure is produced. The droid parameter is set
if the structure was built by a droid. It is not triggered for building theft
(check ```eventObjectTransfer``` for that).

## eventStructureReady(structure)

An event that is run every time a structure is ready to perform some
special ability. It will only fire once, so if the time is not right,
register your own timer to keep checking.

## eventAttacked(victim, attacker)

An event that is run when an object belonging to the script's controlling player is
attacked. The attacker parameter may be either a structure or a droid.

## eventResearched(research, structure, player)

An event that is run whenever a new research is available. The structure
parameter is set if the research comes from a research lab owned by the
current player. If an ally does the research, the structure parameter will
be set to null. The player parameter gives the player it is called for.

## eventDestroyed(object)

An event that is run whenever an object is destroyed. Careful passing
the parameter object around, since it is about to vanish!

## eventPickup(feature, droid)

An event that is run whenever a feature is picked up. It is called for
all players / scripts.
Careful passing the parameter object around, since it is about to vanish! (3.2+ only)

## eventObjectSeen(viewer, seen)

An event that is run sometimes when an object, which was marked by an object label,
which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
An event that is run sometimes when an objectm  goes from not seen to seen.
First parameter is **game object** doing the seeing, the next the game
object being seen.

## eventGroupSeen(viewer, group)

An event that is run sometimes when a member of a group, which was marked by a group label,
which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
First parameter is **game object** doing the seeing, the next the id of the group
being seen.

## eventObjectTransfer(object, from)

An event that is run whenever an object is transferred between players,
for example due to a Nexus Link weapon. The event is called after the
object has been transferred, so the target player is in object.player.
The event is called for both players.

## eventChat(from, to, message)

An event that is run whenever a chat message is received. The ```from``` parameter is the
player sending the chat message. For the moment, the ```to``` parameter is always the script
player.

## eventBeacon(x, y, from, to[, message])

An event that is run whenever a beacon message is received. The ```from``` parameter is the
player sending the beacon. For the moment, the ```to``` parameter is always the script player.
Message may be undefined.

## eventBeaconRemoved(from, to)

An event that is run whenever a beacon message is removed. The ```from``` parameter is the
player sending the beacon. For the moment, the ```to``` parameter is always the script player.

## eventSelectionChanged(objects)

An event that is triggered whenever the host player selects one or more game objects.
The ```objects``` parameter contains an array of the currently selected game objects.
Keep in mind that the player may drag and drop select many units at once, select one
unit specifically, or even add more selections to a current selection one at a time.
This event will trigger once for each user action, not once for each selected or
deselected object. If all selected game objects are deselected, ```objects``` will
be empty.

## eventGroupLoss(object, group id, new size)

An event that is run whenever a group becomes empty. Input parameter
is the about to be killed object, the group's id, and the new group size.

## eventArea<label>(droid)

An event that is run whenever a droid enters an area label. The area is then
deactived. Call resetArea() to reactivate it. The name of the event is
eventArea + the name of the label.

## eventDesignCreated(template)

An event that is run whenever a new droid template is created. It is only
run on the client of the player designing the template.

## eventAllianceOffer(from, to)

An event that is called whenever an alliance offer is requested.

## eventAllianceAccepted(from, to)

An event that is called whenever an alliance is accepted.

## eventAllianceBroken(from, to)

An event that is called whenever an alliance is broken.

## eventSyncRequest(req_id, x, y, obj_id, obj_id2)

An event that is called from a script and synchronized with all other scripts and hosts
to prevent desync from happening. Sync requests must be carefully validated to prevent
cheating!

## eventKeyPressed(meta, key)

An event that is called whenever user presses a key in the game, not counting chat
or other pop-up user interfaces. The key values are currently undocumented.
