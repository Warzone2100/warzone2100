CREATE TABLE Body (component_id integer,              -- Unique number to determine the body
                   configuration integer,             -- The shape of the body
                   armour_multiplier integer,         -- How configuration affects armour - Wedge shapes improve armour, square shapes reduce armour
                   size_modifier integer,             -- The larger the target, the easier to hit
                   max_system_points integer,         -- A measure of how many components can be fitted to the droid
                   weapon_slots integer,              -- The number of  weapons that can be fitted to this body type
                   power_output integer,              -- A measure of how much power the body produces
                   size integer,                      -- 
                   kinetic_armour_value integer,      -- A measure of how much protection the armour provides against kinetic weapons
                   explosive_armour_value integer,    -- A measure of how much protection the armour provides against explosive weapons
                   heat_armour_value integer,         -- A measure of how much protection the armour provides against heat weapons
                   misc_armour_value integer,         -- A measure of how much protection the armour provides against misc weapons
                   design integer,                    -- boolean flag to indicate whether this weapon  can be used in the design screen
                   flame_imd text,                    -- Which graphic to use for the bullet trail
                   UNIQUE(component_id)
);
