# Elements common to all stats structures
struct BASE
    %abstract;
    %suffix "_STATS";
    %macro;
    %macroprefix "STATS_";

    # Unique ID of the item
    C-only-field UDWORD ref;

    # Unique language independant name that can be used to identify a specific
    # stats instance
    string unique       pName;
end;

# if any types are added BEFORE 'COMP_BODY' - then Save/Load Game will have to
# be  altered since it loops through the components from 1->MAX_COMP
enum COMPONENT_TYPE
    %max "COMP_NUMCOMPONENTS";
    %valprefix "COMP_";

    UNKNOWN
    BODY
    BRAIN
    PROPULSION
    REPAIRUNIT
    ECM
    SENSOR
    CONSTRUCT
    WEAPON
end;

# Stats common to all (droid?) components
struct COMPONENT
    %inherit BASE;

    # Power required to build this component
    UDWORD              buildPower;

    # Build points (which are rate-limited in the construction units) required
    # to build this component.
    UDWORD              buildPoints;

    # Weight of this component
    UDWORD              weight;

    # Body points of this component
    UDWORD              body;

    # Indicates whether this component is "designable" and can thus be used in
    # the design screen.
    bool                designable;

    # The "base" IMD model representing this component in 3D space.
    IMD_model optional  pIMD;
end;

# LOC used for holding locations for Sensors and ECM's
enum LOC
    DEFAULT
    TURRET
end;

# SIZE used for specifying body size
enum BODY_SIZE
    %valprefix "SIZE_";

    LIGHT
    MEDIUM
    HEAVY
    SUPER_HEAVY
end;

# only using KINETIC and HEAT for now
enum WEAPON_CLASS
    %max "WC_NUM_WEAPON_CLASSES";
    %valprefix "WC_";

    # Bullets, etc.
    KINETIC

    # Rockets, etc. - classed as KINETIC now to save space in DROID
    #EXPLOSIVE

    # Laser, etc.
    HEAT

    # others we haven't thought of! - classed as HEAT now to save space in DROID
    #WC_MISC
end;

# weapon subclasses used to define which weapons are affected by weapon upgrade
# functions
#
# Watermelon:added a new subclass to do some tests
enum WEAPON_SUBCLASS
    %max "WSC_NUM_WEAPON_SUBCLASSES";
    %valprefix "WSC_";

    MGUN
    CANNON
    #ARTILLARY
    MORTARS
    MISSILE
    ROCKET
    ENERGY
    GAUSS
    FLAME
    #CLOSECOMBAT
    HOWITZERS
    ELECTRONIC
    AAGUN
    SLOWMISSILE
    SLOWROCKET
    LAS_SAT
    BOMB
    COMMAND
    EMP

    # Counter missile
    COUNTER
end;

# Used to define which projectile model to use for the weapon.
enum MOVEMENT_MODEL
    %max "NUM_MOVEMENT_MODEL";
    %valprefix "MM_";

    DIRECT
    INDIRECT
    HOMINGDIRECT
    HOMINGINDIRECT
    ERRATICDIRECT
    SWEEP
end;

# Used to modify the damage to a propuslion type (or structure) based on
# weapon.
enum WEAPON_EFFECT
    %max "WE_NUMEFFECTS";
    %valprefix "WE_";

    ANTI_PERSONNEL
    ANTI_TANK
    BUNKER_BUSTER
    ARTILLERY_ROUND
    FLAMER
    ANTI_AIRCRAFT
end;

# Sides used for droid impact
enum HIT_SIDE
    %max "NUM_HIT_SIDES";

    FRONT
    REAR
    LEFT
    RIGHT
    TOP
    BOTTOM
end;

# Defines the left and right sides for propulsion IMDs
enum PROP_SIDE
    %max "NUM_PROP_SIDES";
    %valsuffix "_PROP";

    LEFT
    RIGHT
end;

enum PROPULSION_TYPE
    %max "PROPULSION_TYPE_NUM";

    WHEELED
    TRACKED
    LEGGED
    HOVER
    SKI
    LIFT
    PROPELLOR
    HALF_TRACKED
    JUMP
end;

struct PROPULSION
    %inherit COMPONENT;
    %nomacro;

    # Max speed for the droid
    UDWORD          maxSpeed;

    # Type of propulsion used - index into PropulsionTable
    enum PROPULSION_TYPE propulsionType;
end;

enum SENSOR_TYPE
    %valprefix "";

    STANDARD_SENSOR
    INDIRECT_CB_SENSOR
    VTOL_CB_SENSOR
    VTOL_INTERCEPT_SENSOR

    # Works as all of the above together! - new for updates
    SUPER_SENSOR
end;

struct SENSOR
    %inherit COMPONENT;
    %nomacro;

    # Sensor range.
    UDWORD          range;

    # Sensor power (put against ecm power).
    UDWORD          power;

    # specifies whether the Sensor is default or for the Turret.
    UDWORD          location;

    # used for combat
    enum SENSOR_TYPE type;

    # Time delay before the associated weapon droids 'know' where the attack is
    # from.
    UDWORD          time;

    # The turret mount to use.
    IMD_model       pMountGraphic;
end;

struct ECM
    %inherit COMPONENT;
    %nomacro;

    # ECM range.
    UDWORD          range;

    # ECM power (put against sensor power).
    UDWORD          power;

    # Specifies whether the ECM is default or for the Turret.
    UDWORD          location;

    # The turret mount to use.
    IMD_model       pMountGraphic;
end;

struct REPAIR
    %inherit COMPONENT;
    %nomacro;

    # How much damage is restored to Body Points and armour each Repair Cycle.
    UDWORD          repairPoints;

    # Whether armour can be repaired or not.
    bool            repairArmour;

    # Specifies whether the Repair is default or for the Turret.
    UDWORD          location;

    # Time delay for repair cycle.
    UDWORD          time;

    # The turret mount to use.
    IMD_model       pMountGraphic;
end;

enum FIREONMOVE
    %valprefix "FOM_";

    # no capability - droid must stop
    NO

    # partial capability - droid has 50% chance to hit
    PARTIAL

    # full capability - droid fires normally on move
    YES
end;

struct CONSTRUCT
    %inherit COMPONENT;
    %nomacro;

    # The number of points contributed each cycle
    UDWORD          constructPoints;

    # The turret mount to use
    IMD_model       pMountGraphic;
end;

enum TRAVEL_MEDIUM
    %valprefix "";

    GROUND
    AIR
end;
