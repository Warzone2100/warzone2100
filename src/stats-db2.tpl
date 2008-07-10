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
