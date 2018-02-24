# Globals

This section describes global variables (or 'globals' for short) that are
available from all scripts. You typically cannot write to these variables,
they are read-only.

* ```version``` Current version of the game, set in *major.minor* format.
* ```selectedPlayer``` The player controlled by the client on which the script runs.
* ```gameTime``` The current game time. Updated before every invokation of a script.
* ```modList``` The current loaded mods.
* ```difficulty``` The currently set campaign difficulty, or the current AI's difficulty setting. It will be one of
```EASY```, ```MEDIUM```, ```HARD``` or ```INSANE```.
* ```mapName``` The name of the current map.
* ```tilesetType``` The area name of the map.
* ```baseType``` The type of base that the game starts with. It will be one of ```CAMP_CLEAN```, ```CAMP_BASE``` or ```CAMP_WALLS```.
* ```alliancesType``` The type of alliances permitted in this game. It will be one of ```NO_ALLIANCES```, ```ALLIANCES``` or ```ALLIANCES_TEAMS```.
* ```powerType``` The power level set for this game.
* ```maxPlayers``` The number of active players in this game.
* ```scavengers``` Whether or not scavengers are activated in this game.
* ```mapWidth``` Width of map in tiles.
* ```mapHeight``` Height of map in tiles.
* ```scavengerPlayer``` Index of scavenger player. (3.2+ only)
* ```isMultiplayer``` If the current game is a online multiplayer game or not. (3.2+ only)
* ```me``` The player the script is currently running as.
* ```scriptName``` Base name of the script that is running.
* ```scriptPath``` Base path of the script that is running.
* ```Stats``` A sparse, read-only array containing rules information for game entity types.
(For now only the highest level member attributes are documented here. Use the 'jsdebug' cheat
to see them all.)
These values are defined:
  * ```Body``` Droid bodies
  * ```Sensor``` Sensor turrets
  * ```ECM``` ECM (Electronic Counter-Measure) turrets
  * ```Propulsion``` Propulsions
  * ```Repair``` Repair turrets (not used, incidentally, for repair centers)
  * ```Construct``` Constructor turrets (eg for trucks)
  * ```Brain``` Brains
  * ```Weapon``` Weapon turrets
  * ```WeaponClass``` Defined weapon classes
  * ```Building``` Buildings
* ```Upgrades``` A special array containing per-player rules information for game entity types,
which can be written to in order to implement upgrades and other dynamic rules changes. Each item in the
array contains a subset of the sparse array of rules information in the ```Stats``` global.
These values are defined:
  * ```Body``` Droid bodies
  * ```Sensor``` Sensor turrets
  * ```Propulsion``` Propulsions
  * ```ECM``` ECM (Electronic Counter-Measure) turrets
  * ```Repair``` Repair turrets (not used, incidentally, for repair centers)
  * ```Construct``` Constructor turrets (eg for trucks)
  * ```Brain``` Brains
BaseCommandLimit: How many droids a commander can command. CommandLimitByLevel: How many extra droids
a commander can command for each of its rank levels. RankThresholds: An array describing how many
kills are required for this brain to level up to the next rank. To alter this from scripts, you must
set the entire array at once. Setting each item in the array will not work at the moment.
  * ```Weapon``` Weapon turrets
  * ```Building``` Buildings
* ```groupSizes``` A sparse array of group sizes. If a group has never been used, the entry in this array will
be undefined.
* ```playerData``` An array of information about the players in a game. Each item in the array is an object
containing the following variables:
  * ```difficulty``` (see ```difficulty``` global constant)
  * ```colour``` number describing the colour of the player
  * ```position``` number describing the position of the player in the game's setup screen
  * ```isAI``` whether the player is an AI (3.2+ only)
  * ```isHuman``` whether the player is human (3.2+ only)
  * ```name``` the name of the player (3.2+ only)
  * ```team``` the number of the team the player is part of
* ```derrickPositions``` An array of derrick starting positions on the current map. Each item in the array is an
object containing the x and y variables for a derrick.
* ```startPositions``` An array of player start positions on the current map. Each item in the array is an
object containing the x and y variables for a player start position.
