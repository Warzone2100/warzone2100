CREATE TABLE Weapons (component_id integer,              -- 
                      short_range integer,               -- Max distance to target for short range shot
                      long_range integer,                -- Max distance to target for long range shot
                      direct_fire integer,               -- boolean Whether the weapon shoots over terrain or not True =Direct Fire, False = Indirect Fire NOT USED ANYMORE
                      short_range_accuracy integer,      -- Base chance of hitting target within short range
                      long_range_accuracy integer,       -- Base chance of hitting target within long range
                      rate_of_fire integer,              -- Rate of fire in milli seconds
                      explosions_per_shot integer,       -- The number of explosions to occur per shot
                      rounds_per_salvo integer,          -- The number of rounds in the salvo, 0 = no salvo
                      reload_time_per_salvo integer,     -- Time to reload for salvo, 0 = no salvo
                      damage integer,                    -- Amount of damage the weapon causes per hit
                      explosive_radius integer,          -- 0 = No radius
                      explosive_hit_chance integer,      -- The base chance to hit targets within the explosive radius
                      explosive_damage integer,          -- The base amount of damage applied to targets within the primary blast area
                      incendiary_burn_time integer,      -- How long the round burns, 0 = No burn
                      incendiary_damage integer,         -- How much damage is done to a target each burn cycle
                      incendiary_burn_radius integer,    -- The burn radius of the round
                      direct_fire_life integer,          -- How long a direct fire weapon is visible for - measured in 1/1000 sec
                      radius_life integer,               -- How long a blast radius is visible for
                      flight_speed integer,              -- How fast the a  round travels
                      indirect_fire_trajectory integer,  -- How high the round travels
                      recoil_value integer,              -- A value to compare with the weight of the droid
                      fire_on_move text,                 -- Flags whether the droid can fire whilst moving (yes, partial, no)
                      weapon_class text,                 -- The damage type of the weapon (kinetic, explosive, heat, misc.)
                      weapon_subclass_id integer,        -- The type used for weapon upgrades
                      bullet_movement_id integer,        -- defines which movement model to use for the bullet
                      bullet_effect_id integer,          -- defines which warhead is attached - used to compare
                      homing_round integer,              -- boolean flag to specify whether the round is homing or not NOT USED ANYMORE
                      min_range integer,                 -- Minimum range for the weapon - limits how near to the target the weapon can be
                      rotate integer,                    -- An amount to specify how much the weapon can be rotated or not 0 = none
                      max_elevation integer,             -- Max amount the turret can rotate up above the horizontal
                      min_elevation integer,             -- Min amount the turret can rotate below above the horizontal
                      surface_to_air integer,            -- A number between 0 and 100 to indicate accuracy in air
                      face_player integer,               -- boolean flag to specify whether the weapon effect is to be rotated to face the viewer or not - the explosion
                      face_player_in_flight integer,     -- boolean flag to specify whether the weapon effect is to be rotated to face the viewer or not - the inflight graphic
                      vtol_attack_runs integer,          -- Number of attack runs for a VTOL vehicle with this weapon
                      effect_size integer,               -- size of the weapon effect 100% = normal, 50% = half size
                      design integer,                    -- boolean flag to indicate whether this weapon  can be used in the design screen
                      light_world integer,               -- boolean flag to indicate whether the effect lights up the world or not
                      mount_graphic text,                -- Which graphic to use for the mount
                      muzzle_graphic text,               -- Which graphic to use for the muzzle effect
                      in_flight_graphic text,            -- Which graphic to use for In Flight
                      target_hit_graphic text,           -- Which graphic to use when hitting target
                      target_miss_graphic text,          -- Which graphic to use when missing target
                      water_hit_graphic text,            -- Which graphic to use when hitting water
                      trail_graphic text,                -- Which graphic to use for the bullet trail
                      UNIQUE(component_id)
);
