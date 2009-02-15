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
    %csv-field 1;
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
    %csv-field 3;
    UDWORD              buildPower;

    # Build points (which are rate-limited in the construction units) required
    # to build this component.
    %csv-field 4;
    UDWORD              buildPoints;

    # Weight of this component
    %csv-field 5;
    UDWORD              weight;

    # Body points of this component
    %csv-field 8;
    UDWORD              body;

    # Indicates whether this component is "designable" and can thus be used in
    # the design screen.
    %csv-field last;
    bool                designable;

    # The "base" IMD model representing this component in 3D space.
    %csv-field 9;
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
    %string "KINETIC";
    %string "EXPLOSIVE";
    KINETIC

    # Rockets, etc. - classed as KINETIC now to save space in DROID
    #EXPLOSIVE

    # Laser, etc.
    %string "HEAT";
    %string "MISC";
    HEAT

    # others we haven't thought of! - classed as HEAT now to save space in DROID
    #MISC
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

    %string "HOMING-DIRECT";
    HOMINGDIRECT

    %string "HOMING-INDIRECT";
    HOMINGINDIRECT

    %string "ERRATIC-DIRECT";
    ERRATICDIRECT

    SWEEP
end;

# Used to modify the damage to a propuslion type (or structure) based on
# weapon.
enum WEAPON_EFFECT
    %max "WE_NUMEFFECTS";
    %valprefix "WE_";

    %string "ANTI PERSONNEL";
    ANTI_PERSONNEL

    %string "ANTI TANK";
    ANTI_TANK

    %string "BUNKER BUSTER";
    BUNKER_BUSTER

    %string "ARTILLERY ROUND";
    ARTILLERY_ROUND

    FLAMER

    %string "ANTI AIRCRAFT";
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

    %string "Wheeled";
    WHEELED

    %string "Tracked";
    TRACKED

    %string "Legged";
    LEGGED

    %string "Hover";
    HOVER

    %string "Ski";
    SKI

    %string "Lift";
    LIFT

    %string "Propellor";
    PROPELLOR

    %string "Half-Tracked";
    HALF_TRACKED

    %string "Jump";
    JUMP
end;

struct PROPULSION
    %inherit COMPONENT;
    %nomacro;
    %loadFunc "loadPropulsionStatsFromDB";
    %preLoadTable rowCount
        if (!statsAllocPropulsion($rowCount))
            ABORT;
    end;
    %postLoadRow curRow rowNum
        $curRow->ref = REF_PROPULSION_START + $rowNum;

        // save the stats
        statsSetPropulsion($curRow, $rowNum);

        // set the max stat values for the design screen
        if ($curRow->designable)
        {
            setMaxPropulsionSpeed($curRow->maxSpeed);
            //setMaxComponentWeight($curRow->weight);
        }
    end;
    %csv-file "propulsion.txt";

    # Max speed for the droid
    %csv-field 11;
    UDWORD          maxSpeed;

    # Type of propulsion used - index into PropulsionTable
    %csv-field 10;
    enum PROPULSION_TYPE propulsionType;
end;

enum SENSOR_TYPE
    %valprefix "";
    %valsuffix "_SENSOR";

    STANDARD

    %string "INDIRECT CB";
    INDIRECT_CB

    %string "VTOL CB";
    VTOL_CB

    %string "VTOL INTERCEPT";
    VTOL_INTERCEPT

    # Works as all of the above together! - new for updates
    SUPER
end;

struct SENSOR
    %inherit COMPONENT;
    %nomacro;
    %loadFunc "loadSensorStatsFromDB";
    %preLoadTable rowCount
        if (!statsAllocSensor($rowCount))
            ABORT;
    end;
    %postLoadRow curRow rowNum
        $curRow->ref = REF_SENSOR_START + $rowNum;

        // save the stats
        statsSetSensor($curRow, $rowNum);

        // set the max stat values for the design screen
        if ($curRow->designable)
        {
            setMaxSensorRange($curRow->range);
            setMaxSensorPower($curRow->power);
            setMaxComponentWeight($curRow->weight);
        }
    end;
    %csv-file "sensor.txt";

    # Sensor range.
    %csv-field 11;
    UDWORD          range;

    # Sensor power (put against ecm power).
    %csv-field 15;
    UDWORD          power;

    # specifies whether the Sensor is default or for the Turret.
    %csv-field 12;
    enum LOC        location;

    # used for combat
    %csv-field 13;
    enum SENSOR_TYPE type;

    # Time delay before the associated weapon droids 'know' where the attack is
    # from.
    %csv-field 14;
    UDWORD          time;

    # The turret mount to use.
    %csv-field 10;
    IMD_model optional pMountGraphic;
end;

struct ECM
    %inherit COMPONENT;
    %nomacro;
    %csv-file "ecm.txt";

    # ECM range.
    %csv-field 13;
    UDWORD          range;

    # ECM power (put against sensor power).
    %csv-field 12;
    UDWORD          power;

    # Specifies whether the ECM is default or for the Turret.
    %csv-field 11;
    UDWORD          location;

    # The turret mount to use.
    %csv-field 10;
    IMD_model optional pMountGraphic;
end;

struct REPAIR
    %inherit COMPONENT;
    %nomacro;
    # FIXME: UDWORD COMPONENT::body; doesn't exist for this component
    #%csv-file "repair.txt";
    %csv-field-override pIMD 10;

    # How much damage is restored to Body Points and armour each Repair Cycle.
    %csv-field 12;
    UDWORD          repairPoints;

    # Whether armour can be repaired or not.
    %csv-field 8;
    bool            repairArmour;

    # Specifies whether the Repair is default or for the Turret.
    %csv-field 9;
    UDWORD          location;

    # Time delay for repair cycle.
    %csv-field 13;
    UDWORD          time;

    # The turret mount to use.
    %csv-field 11;
    IMD_model optional pMountGraphic;
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

struct WEAPON
    %inherit COMPONENT;
    %nomacro;
    %fetchRowById Row Id
        $Row = statsGetWeapon($Id - 1);
    end;
    %csv-file "weapons.txt";

    # WEAPON's CSV layout differs from the rest for this field
    %csv-field-override designable 52;

    # Max distance to target for short range shot
    %csv-field 17;
    UDWORD          shortRange;
    # Max distance to target for long range shot
    %csv-field 18;
    UDWORD          longRange;
    # Min distance to target for shot
    %csv-field 47;
    UDWORD          minRange;
    # Chance to hit at short range
    %csv-field 19;
    UDWORD          shortHit;
    # Chance to hit at long range
    %csv-field 20;
    UDWORD          longHit;
    # Time between each weapon fire
    %csv-field 21;
    UDWORD          firePause;
    # The number of explosions per shot
    %csv-field 22;
    UDWORD          numExplosions;
    # The number of rounds per salvo(magazine)
    %csv-field 23;
    UBYTE           numRounds;
    # Time to reload the round of ammo (salvo fire)
    %csv-field 24;
    UDWORD          reloadTime;
    # How much damage the weapon causes
    %csv-field 25;
    UDWORD          damage;
    # Basic blast radius of weapon
    %csv-field 26;
    UDWORD          radius;
    # Chance to hit in the blast radius
    %csv-field 27;
    UDWORD          radiusHit;
    # Damage done in the blast radius
    %csv-field 28;
    UDWORD          radiusDamage;
    # How long the round burns
    %csv-field 29;
    UDWORD          incenTime;
    # Damage done each burn cycle
    %csv-field 30;
    UDWORD          incenDamage;
    # Burn radius of the round
    %csv-field 31;
    UDWORD          incenRadius;
    # speed ammo travels at
    %csv-field 34;
    UDWORD          flightSpeed;
    # how high the ammo travels for indirect fire
    %csv-field 35;
    UDWORD          indirectHeight;
    # indicates whether the droid has to stop before firing
    %csv-field 36;
    enum FIREONMOVE fireOnMove;
    # the class of weapon
    %csv-field 37;
    enum WEAPON_CLASS weaponClass;
    # the subclass to which the weapon belongs
    %csv-field 38;
    enum WEAPON_SUBCLASS weaponSubClass;

    # which projectile model to use for the bullet
    %csv-field 39;
    enum MOVEMENT_MODEL movementModel;
    # which type of warhead is associated with the weapon
    %csv-field 40;
    enum WEAPON_EFFECT weaponEffect;
    # used to compare with weight to see if recoils or not
    %csv-field 46;
    UDWORD          recoilValue;
    # amount the weapon(turret) can rotate 0 = none
    %csv-field 41;
    UBYTE           rotate;
    # max amount the turret can be elevated up
    %csv-field 42;
    UBYTE           maxElevation;
    # min amount the turret can be elevated down
    %csv-field 43;
    SBYTE           minElevation;
    # flag to make the (explosion) effect face the player when drawn
    %csv-field 44;
    UBYTE           facePlayer;
    # flag to make the inflight effect face the player when drawn
    %csv-field 45;
    UBYTE           faceInFlight;
    # size of the effect 100 = normal, 50 = half etc
    %csv-field 49;
    UBYTE           effectSize;
    # flag to indicate whether the effect lights up the world
    %csv-field 48;
    bool            lightWorld;
    # indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
    %csv-field 50;
    UBYTE           surfaceToAir;
    # number of attack runs a VTOL droid can do with this weapon
    %csv-field 51;
    UBYTE           vtolAttackRuns;
    # flag to indicate whether pentrate droid or not
    %csv-field 53;
    bool            penetrate;

    # Graphics control stats

    # How long a direct fire weapon is visible. Measured in 1/100 sec.
    %csv-field 32;
    UDWORD          directLife;
    # How long a blast radius is visible
    %csv-field 33;
    UDWORD          radiusLife;

    # Graphics used for the weapon

    # The turret mount to use
    %csv-field 10;
    IMD_model optional pMountGraphic;
    # The muzzle flash
    %csv-field 11;
    IMD_model        pMuzzleGraphic;
    # The ammo in flight
    %csv-field 12;
    IMD_model        pInFlightGraphic;
    # The ammo hitting a target
    %csv-field 13;
    IMD_model        pTargetHitGraphic;
    # The ammo missing a target
    %csv-field 14;
    IMD_model        pTargetMissGraphic;
    # The ammo hitting water
    %csv-field 15;
    IMD_model        pWaterHitGraphic;
    # The trail used for in flight
    %csv-field 16;
    IMD_model optional pTrailGraphic;

    # Audio
    C-only-field SDWORD           iAudioFireID;
    C-only-field SDWORD           iAudioImpactID;
end;

struct BRAIN
    %inherit COMPONENT;
    %nomacro;
    # FIXME: UDWORD COMPONENT::body; doesn't exist for this component
    # FIXME: bool COMPONENT::designable; doesn't exist for this component
    #%csv-file "brain.txt";

    # Program capacity
    %csv-field 9;
    UDWORD              progCap;

    # Weapon stats associated with this brain - for Command Droids
    struct WEAPON       psWeaponStat;
end;

struct CONSTRUCT
    %inherit COMPONENT;
    %nomacro;
    %loadFunc "loadConstructStatsFromDB";
    %preLoadTable rowCount
        if (!statsAllocConstruct($rowCount))
            ABORT;
    end;
    %postLoadRow curRow rowNum
        $curRow->ref = REF_CONSTRUCT_START + $rowNum;

        // save the stats
        statsSetConstruct($curRow, $rowNum);

        // set the max stat values for the design screen
        if ($curRow->designable)
        {
            setMaxConstPoints($curRow->constructPoints);
            setMaxComponentWeight($curRow->weight);
        }
    end;
    %csv-file "construction.txt";

    # The number of points contributed each cycle
    %csv-field 11;
    UDWORD          constructPoints;

    # The turret mount to use
    %csv-field 10;
    IMD_model optional pMountGraphic;
end;

enum TRAVEL_MEDIUM
    %valprefix "";

    GROUND
    AIR
end;
