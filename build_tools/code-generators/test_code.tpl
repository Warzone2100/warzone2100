struct BASE
    %abstract;
    %prefix "STATS_";

    # Unique language independant name that can be used to identify a specific
    # stats instance
    string unique       id;

    # short name, describing the component, must be translateable
    string optional     name;
end;

# Enumerates the different technology levels
enum TECH_LEVEL
    ONE
    TWO
    THREE
end;

# Represents a researchable component statistic
struct COMPONENT
    %inherit BASE;

    # Technology level(s) of this component
    set TECH_LEVEL      level;

    # Power required to build this component
    real                buildPower;

    # Build points (which are rate-limited in the construction units) required
    # to build this component.
    real                buildPoints;

    # Weight of this component
    real                weight;

    # Indicates whether this component is "designable" and can thus be used in
    # the design screen.
    bool                designable;

    # @TODO devise some kind of type to refer to IMD models; perhaps just
    #       filenames will suffice?
    #
    #       We'll name it IMD_model for now.
    #
    # The "base" IMD model representing this component in 3D space.
    IMD_model optional  baseModel;
end;

# Will contain all data associated with a body
struct BODY
    %inherit COMPONENT;
    %loadFunc "loadBodyStatsFromDB";
    %preLoadTable maxId;
        if (!statsAllocBody($maxId))
            ABORT;
    end;
    %postLoadRow curRow curId;
        // set the max stat values for the design screen
        if ($curRow->designable)
        {
            // use front armour value to prevent bodyStats corrupt problems
            setMaxBodyArmour($curRow->armourValue[HIT_SIDE_FRONT][WC_KINETIC]);
            setMaxBodyArmour($curRow->armourValue[HIT_SIDE_FRONT][WC_HEAT]);
            setMaxBodyPower($curRow->powerOutput);
            setMaxBodyPoints($curRow->body);
            setMaxComponentWeight($curRow->weight);
        }

        // save the stats
        statsSetBody($curRow, $curId - 1);
    end;

    # The number of available weaponSlots slots on the body
    count           weaponSlots;

    # Engine output of this body's engine
    real            powerOutput;
end;
