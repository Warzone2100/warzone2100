# Game objects

This section describes various **game objects** defined by the script interface,
and which are both accepted by functions and returned by them. Changing the
fields of a **game object** has no effect on the game before it is passed to a
function that does something with the **game object**.

## Research

Describes a research item. The following properties are defined:

* ```power``` Number of power points needed for starting the research.
* ```points``` Number of research points needed to complete the research.
* ```started``` A boolean saying whether or not this research has been started by current player or any of its allies.
* ```done``` A boolean saying whether or not this research has been completed.
* ```name``` A string containing the full name of the research.
* ```id``` A string containing the index name of the research.
* ```type``` The type will always be ```RESEARCH_DATA```.

## Structure

Describes a structure (building). It inherits all the properties of the base object (see below).
In addition, the following properties are defined:

* ```status``` The completeness status of the structure. It will be one of ```BEING_BUILT``` and ```BUILT```.
* ```type``` The type will always be ```STRUCTURE```.
* ```cost``` What it would cost to build this structure. (3.2+ only)
* ```stattype``` The stattype defines the type of structure. It will be one of ```HQ```, ```FACTORY```, ```POWER_GEN```,
```RESOURCE_EXTRACTOR```, ```LASSAT```, ```DEFENSE```, ```WALL```, ```RESEARCH_LAB```, ```REPAIR_FACILITY```,
```CYBORG_FACTORY```, ```VTOL_FACTORY```, ```REARM_PAD```, ```SAT_UPLINK```, ```GATE``` and ```COMMAND_CONTROL```.
* ```modules``` If the stattype is set to one of the factories, ```POWER_GEN``` or ```RESEARCH_LAB```, then this property is set to the
number of module upgrades it has.
* ```canHitAir``` True if the structure has anti-air capabilities. (3.2+ only)
* ```canHitGround``` True if the structure has anti-ground capabilities. (3.2+ only)
* ```isSensor``` True if the structure has sensor ability. (3.2+ only)
* ```isCB``` True if the structure has counter-battery ability. (3.2+ only)
* ```isRadarDetector``` True if the structure has radar detector ability. (3.2+ only)
* ```range``` Maximum range of its weapons. (3.2+ only)
* ```hasIndirect``` One or more of the structure's weapons are indirect. (3.2+ only)

## Feature

Describes a feature (a **game object** not owned by any player). It inherits all the properties of the base object (see below).
In addition, the following properties are defined:
* ```type``` It will always be ```FEATURE```.
* ```stattype``` The type of feature. Defined types are ```OIL_RESOURCE```, ```OIL_DRUM``` and ```ARTIFACT```.
* ```damageable``` Can this feature be damaged?

## Droid

Describes a droid. It inherits all the properties of the base object (see below).
In addition, the following properties are defined:

* ```type``` It will always be ```DROID```.
* ```order``` The current order of the droid. This is its plan. The following orders are defined:
  * ```DORDER_ATTACK``` Order a droid to attack something.
  * ```DORDER_MOVE``` Order a droid to move somewhere.
  * ```DORDER_SCOUT``` Order a droid to move somewhere and stop to attack anything on the way.
  * ```DORDER_BUILD``` Order a droid to build something.
  * ```DORDER_HELPBUILD``` Order a droid to help build something.
  * ```DORDER_LINEBUILD``` Order a droid to build something repeatedly in a line.
  * ```DORDER_REPAIR``` Order a droid to repair something.
  * ```DORDER_RETREAT``` Order a droid to retreat back to HQ.
  * ```DORDER_PATROL``` Order a droid to patrol.
  * ```DORDER_DEMOLISH``` Order a droid to demolish something.
  * ```DORDER_EMBARK``` Order a droid to embark on a transport.
  * ```DORDER_DISEMBARK``` Order a transport to disembark its units at the given position.
  * ```DORDER_FIRESUPPORT``` Order a droid to fire at whatever the target sensor is targeting. (3.2+ only)
  * ```DORDER_COMMANDERSUPPORT``` Assign the droid to a commander. (3.2+ only)
  * ```DORDER_STOP``` Order a droid to stop whatever it is doing. (3.2+ only)
  * ```DORDER_RTR``` Order a droid to return for repairs. (3.2+ only)
  * ```DORDER_RTB``` Order a droid to return to base. (3.2+ only)
  * ```DORDER_HOLD``` Order a droid to hold its position. (3.2+ only)
  * ```DORDER_REARM``` Order a VTOL droid to rearm. If given a target, will go to specified rearm pad. If not, will go to nearest rearm pad. (3.2+ only)
  * ```DORDER_OBSERVE``` Order a droid to keep a target in sensor view. (3.2+ only)
  * ```DORDER_RECOVER``` Order a droid to pick up something. (3.2+ only)
  * ```DORDER_RECYCLE``` Order a droid to factory for recycling. (3.2+ only)
* ```action``` The current action of the droid. This is how it intends to carry out its plan. The
C++ code may change the action frequently as it tries to carry out its order. You never want to set
the action directly, but it may be interesting to look at what it currently is.
* ```droidType``` The droid's type. The following types are defined:
  * ```DROID_CONSTRUCT``` Trucks and cyborg constructors.
  * ```DROID_WEAPON``` Droids with weapon turrets, except cyborgs.
  * ```DROID_PERSON``` Non-cyborg two-legged units, like scavengers.
  * ```DROID_REPAIR``` Units with repair turret, including repair cyborgs.
  * ```DROID_SENSOR``` Units with sensor turret.
  * ```DROID_ECM``` Unit with ECM jammer turret.
  * ```DROID_CYBORG``` Cyborgs with weapons.
  * ```DROID_TRANSPORTER``` Cyborg transporter.
  * ```DROID_SUPERTRANSPORTER``` Droid transporter.
  * ```DROID_COMMAND``` Commanders.
* ```group``` The group this droid is member of. This is a numerical ID. If not a member of any group, will be set to \emph{null}.
* ```armed``` The percentage of weapon capability that is fully armed. Will be \emph{null} for droids other than VTOLs.
* ```experience``` Amount of experience this droid has, based on damage it has dealt to enemies.
* ```cost``` What it would cost to build the droid. (3.2+ only)
* ```isVTOL``` True if the droid is VTOL. (3.2+ only)
* ```canHitAir``` True if the droid has anti-air capabilities. (3.2+ only)
* ```canHitGround``` True if the droid has anti-ground capabilities. (3.2+ only)
* ```isSensor``` True if the droid has sensor ability. (3.2+ only)
* ```isCB``` True if the droid has counter-battery ability. (3.2+ only)
* ```isRadarDetector``` True if the droid has radar detector ability. (3.2+ only)
* ```hasIndirect``` One or more of the droid's weapons are indirect. (3.2+ only)
* ```range``` Maximum range of its weapons. (3.2+ only)
* ```body``` The body component of the droid. (3.2+ only)
* ```propulsion``` The propulsion component of the droid. (3.2+ only)
* ```weapons``` The weapon components of the droid, as an array. Contains 'name', 'id', 'armed' percentage and 'lastFired' properties. (3.2+ only)
* ```cargoCapacity``` Defined for transporters only: Total cargo capacity (number of items that will fit may depend on their size). (3.2+ only)
* ```cargoSpace``` Defined for transporters only: Cargo capacity left. (3.2+ only)
* ```cargoCount``` Defined for transporters only: Number of individual \emph{items} in the cargo hold. (3.2+ only)
* ```cargoSize``` The amount of cargo space the droid will take inside a transport. (3.2+ only)

## Base Object

Describes a basic object. It will always be a droid, structure or feature, but sometimes the
difference does not matter, and you can treat any of them simply as a basic object. These
fields are also inherited by the droid, structure and feature objects.
The following properties are defined:

* ```type``` It will be one of ```DROID```, ```STRUCTURE``` or ```FEATURE```.
* ```id``` The unique ID of this object.
* ```x``` X position of the object in tiles.
* ```y``` Y position of the object in tiles.
* ```z``` Z (height) position of the object in tiles.
* ```player``` The player owning this object.
* ```selected``` A boolean saying whether 'selectedPlayer' has selected this object.
* ```name``` A user-friendly name for this object.
* ```health``` Percentage that this object is damaged (where 100 means not damaged at all).
* ```armour``` Amount of armour points that protect against kinetic weapons.
* ```thermal``` Amount of thermal protection that protect against heat based weapons.
* ```born``` The game time at which this object was produced or came into the world. (3.2+ only)

## Template

Describes a template type. Templates are droid designs that a player has created.
The following properties are defined:

* ```id``` The ID of this object.
* ```name``` Name of the template.
* ```cost``` The power cost of the template if put into production.
* ```droidType``` The type of droid that would be created.
* ```body``` The name of the body type.
* ```propulsion``` The name of the propulsion type.
* ```brain``` The name of the brain type.
* ```repair``` The name of the repair type.
* ```ecm``` The name of the ECM (electronic counter-measure) type.
* ```construct``` The name of the construction type.
* ```weapons``` An array of weapon names attached to this template.
