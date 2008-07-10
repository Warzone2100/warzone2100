# if any types are added BEFORE 'COMP_BODY' - then Save/Load Game will have to
# be  altered since it loops through the components from 1->MAX_COMP
enum COMPONENT_TYPE
    %max "NUMCOMPONENTS";
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
    %max "NUM_WEAPON_CLASSES";
    %valprefix "WC_";

    # Bullets, etc.
    KINETIC

    # Rockets, etc. - classed as KINETIC now to save space in DROID
    #EXPLOSIVE

    # Laser, etc.
    HEAT

    # others we haven't thought of! - classed as HEAT now to save space in DROID
    #WC_MISC,                ///< 
end;

# weapon subclasses used to define which weapons are affected by weapon upgrade
# functions
#
# Watermelon:added a new subclass to do some tests
enum WEAPON_SUBCLASS
    %max "NUM_WEAPON_SUBCLASSES";
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

enum PROPULSION_TYPE
    %max "NUM";
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

enum FIREONMOVE
    %valprefix "FOM_";

    # no capability - droid must stop
    NO

    # partial capability - droid has 50% chance to hit
    PARTIAL

    # full capability - droid fires normally on move
    YES
end;

enum TRAVEL_MEDIUM
    %valprefix "";

    GROUND
    AIR
end;
